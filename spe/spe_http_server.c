#include "spe_http_server.h"
#include "spe_server.h"

#define HTTP_READHEADER 1
#define HTTP_CLOSE      2

static int
httpParserUrl(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  spe_string_copyb(request->url, at, length);
  return 0;
}

static int
httpHeaderField(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  spe_string_copyb(request->Buffer, at, length);
  return 0;
}

static int
httpHeaderValue(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  spe_string_t* value = spe_string_create(16);
  spe_string_copyb(value, at, length);
  SpeMapSet(request->header, request->Buffer->data, value);
  return 0;
}

static struct http_parser_settings parser_settings = {
  NULL,
  httpParserUrl,
  httpHeaderField,
  httpHeaderValue,
  NULL,
  NULL,
  NULL,
};

static void
driver_machine(void* arg) {
  SpeHttpRequest_t* request = arg;
  if (request->conn->Error || request->conn->Closed) {
    SpeConnDestroy(request->conn);
    free(request);
    return;
  }
  int res;
  switch (request->status) {
    case HTTP_READHEADER:
      res = http_parser_execute(&request->parser, &parser_settings, request->conn->Buffer->data, 
          request->conn->Buffer->len);
      if (res == 0 || res != request->conn->Buffer->len) {
        SPE_LOG_ERR("http_parse_execute header error");
        SpeHttpRequestDestroy(request);
        return;
      }
      SpeMapItem_t* item = SpeMapNext(request->header, NULL);
      while(item) {
        fprintf(stdout, "header %s:%s\n", item->key, ((spe_string_t*)item->obj)->data);
        item = SpeMapNext(request->header, item);
      }
      request->status = HTTP_CLOSE;
      SpeConnWrite(request->conn, request->url);
      SpeConnWrites(request->conn, "\n");
      SpeConnFlush(request->conn);
      break;
    case HTTP_CLOSE:
      SpeHttpRequestDestroy(request);
      break;
  }
}

/*
===================================================================================================
SpeHttpRequestCreate
===================================================================================================
*/
SpeHttpRequest_t*
SpeHttpRequestCreate(SpeConn_t* conn) {
  SpeHttpRequest_t* request = calloc(1, sizeof(SpeHttpRequest_t));
  if (!request) {
    SPE_LOG_ERR("request calloc error");
    return NULL;
  }
  http_parser_init(&request->parser, HTTP_REQUEST);
  request->parser.data  = request;
  request->url          = spe_string_create(16);
  request->header       = SpeMapCreate(31, (SpeMapHandler)spe_string_destroy);
  request->Buffer       = spe_string_create(0);
  request->conn         = conn;
  return request;
}

/*
===================================================================================================
SpeHttpRequestDestroy
===================================================================================================
*/
void
SpeHttpRequestDestroy(SpeHttpRequest_t* request) {
  ASSERT(request);
  SpeConnDestroy(request->conn);
  spe_string_destroy(request->url);
  spe_string_destroy(request->Buffer);
  SpeMapDestroy(request->header);
  free(request);
}

static void
httpHandler(SpeConn_t* conn) {
  SpeHttpRequest_t* request = SpeHttpRequestCreate(conn);
  if (!request) {
    SPE_LOG_ERR("request create Error");
    SpeConnDestroy(conn);
    return;
  }
  request->status       = HTTP_READHEADER;
  conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, request);
  conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, request);
  SpeConnReaduntil(conn, "\r\n\r\n");
}

/*
===================================================================================================
SpeHttpServerInit
===================================================================================================
*/
bool
SpeHttpServerInit(const char* addr, int port) {
  return SpeServerInit(addr, port, httpHandler);
}

/*
===================================================================================================
SpeHttpServerDeinit
===================================================================================================
*/
void
SpeHttpServerDeinit() {
  SpeServerDeinit();
}
