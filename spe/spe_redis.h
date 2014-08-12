#ifndef __SPE_REDIS_H
#define __SPE_REDIS_H

#include "spe_conn.h"
#include "spe_handler.h"
#include "spe_string.h"

struct SpeRedis_s {
  SpeConn_t*    conn;
  const char*   host;
  const char*   port;
  spe_handler_t handler;
  SpeSlist_t*  Buffer;
  SpeSlist_t*  sendBuffer;
  unsigned      blockLeft;
  unsigned      status:7;
  unsigned      Error:1;
};
typedef struct SpeRedis_s SpeRedis_t;

struct SpeRedisPool_s {
  const char* host;
  const char* port;
  unsigned    size;
  unsigned    len;
  SpeRedis_t* poolData[0];
};
typedef struct SpeRedisPool_s SpeRedisPool_t;

extern void
SpeRedisDo(SpeRedis_t* sr, spe_handler_t handler, int nargs, ...);

extern void
SpeRedisGet(SpeRedis_t* sr, spe_handler_t handler, const char* key);

extern void
SpeRedisSet(SpeRedis_t* sr, spe_handler_t handler, const char* key, const char* value);

extern SpeRedis_t*
SpeRedisPoolGet(SpeRedisPool_t* srp);

extern void
SpeRedisPoolPut(SpeRedisPool_t* srp, SpeRedis_t* sr);

extern SpeRedisPool_t*
SpeRedisPoolCreate(const char* host, const char* port, unsigned size);

extern void
SpeRedisPoolDestroy(SpeRedisPool_t* srp);

#endif
