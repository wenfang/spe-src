#include "spe_http_server.h"
#include "spe_conn.h"
#include "spe_server.h"

#define HTTP_READHEADER 1

struct httpConn_s {
  SpeConn_t*  conn;
  unsigned    status; 
};
typedef struct httpConn_s httpConn_t;

static void
driver_machine(void* arg) {
  httpConn_t* httpConn = arg;
  if (httpConn->conn->Error || httpConn->conn->Closed) {
    SpeConnDestroy(httpConn->conn);
    free(httpConn);
    return;
  }
  switch (httpConn->status) {
    case HTTP_READHEADER:
      SpeConnDestroy(httpConn->conn);
      free(httpConn);
      break;
  }
}

static void
httpHandler(SpeConn_t* conn) {
  httpConn_t* httpConn = calloc(1, sizeof(httpConn_t));
  if (!httpConn) {
    SPE_LOG_ERR("httpConn create Error");
    SpeConnDestroy(conn);
    return;
  }
  conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, httpConn);
  conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, httpConn);
  httpConn->conn = conn;
  httpConn->status = HTTP_READHEADER;
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
