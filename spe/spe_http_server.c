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

static struct http_parser_settings parser_settings = {
  NULL,
  httpParserUrl,
  NULL,
  NULL,
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
        SpeConnDestroy(request->conn);
        spe_string_destroy(request->url);
        spe_slist_destroy(request->header);
        free(request);
        return;
      }
      request->status = HTTP_CLOSE;
      SpeConnWrite(request->conn, request->url);
      SpeConnFlush(request->conn);
      break;
    case HTTP_CLOSE:
      SpeConnDestroy(request->conn);
      spe_string_destroy(request->url);
      spe_slist_destroy(request->header);
      free(request);
      break;
  }
}

static void
httpHandler(SpeConn_t* conn) {
  SpeHttpRequest_t* request = calloc(1, sizeof(SpeHttpRequest_t));
  if (!request) {
    SPE_LOG_ERR("request create Error");
    SpeConnDestroy(conn);
    return;
  }
  http_parser_init(&request->parser, HTTP_REQUEST);
  request->url          = spe_string_create(16);
  request->header       = spe_slist_create();
  request->parser.data  = request;
  request->conn         = conn;
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