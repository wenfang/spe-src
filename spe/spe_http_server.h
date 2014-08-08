#ifndef __SPE_HTTP_SERVER_H
#define __SPE_HTTP_SERVER_H

#include "spe_string.h"
#include "spe_conn.h"
#include "spe_map.h"
#include "http_parser.h"
#include <stdbool.h>

struct SpeHttpRequest_s {
  spe_string_t* url;
  SpeMap_t*     header;
  spe_string_t* Buffer;
  SpeConn_t*    conn;
  http_parser   parser;
  unsigned      status;
};
typedef struct SpeHttpRequest_s SpeHttpRequest_t;

typedef void (*SpeHttpHandler)(SpeHttpRequest_t*);

extern bool
SpeHttpRegisterHandler(const char* pattern, SpeHttpHandler handler);

extern bool
SpeHttpServerInit(const char* addr, int port);

extern void
SpeHttpServerDeinit();

#endif
