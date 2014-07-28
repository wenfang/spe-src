#ifndef __SPE_REDIS_H
#define __SPE_REDIS_H

#include "spe_conn.h"
#include "spe_handler.h"
#include "spe_string.h"

struct SpeRedisPool_s;
typedef struct SpeRedisPool_s SpeRedisPool_t;

struct SpeRedis_s {
  spe_conn_t*   conn;
  const char*   host;
  const char*   port;
  spe_handler_t handler;
  spe_slist_t*  sendBuffer;
  spe_slist_t*  recvBuffer;
  unsigned      status:7;
  unsigned      error:1;
};
typedef struct SpeRedis_s SpeRedis_t;

struct SpeRedisPool_s {
  const char* host;
  const char* port;
  unsigned    size;
  unsigned    len;
  SpeRedis_t* poolData[0];
};

extern bool
SpeRedisDo(SpeRedis_t* sr, spe_handler_t handler, int nargs, ...);

extern SpeRedis_t*
SpeRedisGet(SpeRedisPool_t* srp);

extern void
SpeRedisPut(SpeRedisPool_t* srp, SpeRedis_t* sr);

extern SpeRedisPool_t*
SpeRedisPoolCreate(const char* host, const char* port, unsigned size);

extern void
SpeRedisPoolDestroy(SpeRedisPool_t* srp);

#endif
