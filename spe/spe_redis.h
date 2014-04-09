#ifndef __SPE_REDIS_H
#define __SPE_REDIS_H

#include "spe_conn.h"
#include "spe_handler.h"
#include "spe_string.h"

struct spe_redis_s {
  spe_conn_t*   conn;
  const char*   host;
  const char*   port;
  spe_handler_t handler;
  spe_slist_t*  send_buffer;
  spe_slist_t*  recv_buffer;
  unsigned      closed:1;
  unsigned      error:1;
};
typedef struct spe_redis_s spe_redis_t;

extern bool
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler);

extern bool
spe_redis_do(spe_redis_t* sr, spe_handler_t handler, int nargs, ...);

extern spe_redis_t*
spe_redis_create(const char* host, const char* port);

extern void
spe_redis_destroy(spe_redis_t* sr);

#endif
