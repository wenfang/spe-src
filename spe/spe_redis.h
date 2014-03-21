#ifndef __SPE_REDIS_H
#define __SPE_REDIS_H

#include "spe_conn.h"
#include "spe_handler.h"

struct spe_redis_s {
  spe_conn_t* conn;
  const char* host;
  const char* port;
};
typedef struct spe_redis_s spe_redis_t;

extern void
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler);

extern spe_redis_t*
spe_redis_create(const char* host, const char* port);

extern void
spe_redis_destroy(spe_redis_t* sr);

#endif
