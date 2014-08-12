#include "spe_http_server.h"
#include "spe_server.h"
#include "spe_reg.h"

#define HTTP_READHEADER 1
#define HTTP_CLOSE      2

static LIST_HEAD(httpHandlerList);

struct httpHandler_s {
  struct list_head  handlerNode;
  SpeReg_t*         reg;
  SpeHttpHandler    handler;
};
typedef struct httpHandler_s httpHandler_t;

/*
===================================================================================================
SpeHttpRegisterHandler
===================================================================================================
*/
bool
SpeHttpRegisterHandler(const char* pattern, SpeHttpHandler handler) {
  httpHandler_t* httpHandler = calloc(1, sizeof(httpHandler_t));
  if (!httpHandler) {
    SPE_LOG_ERR("httpHandler calloc error");
    return false;
  }
  httpHandler->reg = SpeRegCreate(pattern);
  if (!httpHandler->reg) {
    SPE_LOG_ERR("SpeRegCreate error");
    free(httpHandler);
    return false;
  }
  httpHandler->handler = handler;
  INIT_LIST_HEAD(&httpHandler->handlerNode);
  list_add_tail(&httpHandler->handlerNode, &httpHandlerList);
  return true;
}

/*
===================================================================================================
SpeHttpUnregisterHandler
===================================================================================================
*/
void
SpeHttpUnregisterHandler() {
  httpHandler_t* httpHandler;
  while (!list_empty(&httpHandlerList)) {
    httpHandler = list_first_entry(&httpHandlerList, httpHandler_t, handlerNode);
    list_del(&httpHandler->handlerNode);
    free(httpHandler);
  }
}

static int
httpUrl(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  SpeStringCopyb(request->url, at, length);
  return 0;
}

static int
httpHeaderName(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  SpeStringCopyb(request->headerName, at, length);
  return 0;
}

static int
httpHeaderValue(http_parser* parser, const char* at, size_t length) {
  SpeHttpRequest_t* request = parser->data;
  SpeString_t* headerValue = SpeStringCreate(0);
  SpeStringCopyb(headerValue, at, length);
  SpeMapSet(request->header, request->headerName->data, headerValue);
  return 0;
}

static struct http_parser_settings parser_settings = {
  NULL,
  httpUrl,
  httpHeaderName,
  httpHeaderValue,
  NULL,
  NULL,
  NULL,
};

static void
dispatchHandler(SpeHttpRequest_t* request) {
  httpHandler_t* httpHandler;
  list_for_each_entry(httpHandler, &httpHandlerList, handlerNode) {
    if (!SpeRegSearch(httpHandler->reg, request->url->data)) {
      continue;
    }
    request->Conn->ReadCallback.Handler  = SPE_HANDLER1(httpHandler->handler, request);
    request->Conn->WriteCallback.Handler = SPE_HANDLER1(httpHandler->handler, request);
    httpHandler->handler(request);
    return;
  }
  // No Handler Found!!! destroy
  SpeHttpRequestDestroy(request);
}

static void
driver_machine(void* arg) {
  SpeHttpRequest_t* request = arg;
  if (request->Conn->Error || request->Conn->Closed) {
    SpeConnDestroy(request->Conn);
    free(request);
    return;
  }
  int res;
  switch (request->status) {
    case HTTP_READHEADER:
      res = http_parser_execute(&request->parser, &parser_settings, request->Conn->Buffer->data, 
          request->Conn->Buffer->len);
      if (res == 0 || res != request->Conn->Buffer->len) {
        SPE_LOG_ERR("http_parse_execute header error");
        SpeHttpRequestDestroy(request);
        return;
      }
      dispatchHandler(request);
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
  request->url          = SpeStringCreate(16);
  request->header       = SpeMapCreate(31, (SpeMapHandler)SpeStringDestroy);
  request->headerName   = SpeStringCreate(16);
  request->Conn         = conn;
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
  SpeConnDestroy(request->Conn);
  SpeStringDestroy(request->url);
  SpeMapDestroy(request->header);
  SpeStringDestroy(request->headerName);
  free(request);
}

/*
===================================================================================================
SpeHttpRequestFinish
===================================================================================================
*/
void
SpeHttpRequestFinish(SpeHttpRequest_t* request) {
  ASSERT(request);
  request->Conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, request);
  request->Conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, request);
  request->status = HTTP_CLOSE;
  SpeConnFlush(request->Conn);
}

static void
httpHandler(SpeConn_t* conn) {
  SpeHttpRequest_t* request = SpeHttpRequestCreate(conn);
  if (!request) {
    SPE_LOG_ERR("request create Error");
    SpeConnDestroy(conn);
    return;
  }
  request->status             = HTTP_READHEADER;
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
