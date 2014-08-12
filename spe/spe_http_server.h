#ifndef __SPE_HTTP_SERVER_H
#define __SPE_HTTP_SERVER_H

#include "spe_string.h"
#include "spe_conn.h"
#include "spe_map.h"
#include "http_parser.h"
#include <stdbool.h>

struct SpeHttpRequest_s {
  SpeConn_t*    Conn;
  SpeString_t* url;
  SpeMap_t*     header;
  SpeString_t* headerName;
  http_parser   parser;
  unsigned      status;
  void*         Private;
};
typedef struct SpeHttpRequest_s SpeHttpRequest_t;

typedef void (*SpeHttpHandler)(SpeHttpRequest_t*);

extern bool
SpeHttpRegisterHandler(const char* pattern, SpeHttpHandler handler);

extern void
SpeHttpUnregisterHandler();

extern bool
SpeHttpServerInit(const char* addr, int port);

extern void
SpeHttpServerDeinit();

extern SpeHttpRequest_t*
SpeHttpRequestCreate(SpeConn_t* conn);

extern void
SpeHttpRequestDestroy(SpeHttpRequest_t* request);

extern void
SpeHttpRequestFinish(SpeHttpRequest_t* request);

#endif
