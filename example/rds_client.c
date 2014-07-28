#include "spe.h"
#include <stdio.h>

static SpeRedisPool_t* pool;

static void
on_get(void* arg) {
  GStop = true;
  SpeRedis_t* sr = arg;
  if (sr->Error) {
    fprintf(stderr, "speRedis_t error\n");
    SpeRedisPoolPut(pool, sr);
    return;
  }
  for (int i=0; i<sr->recvBuffer->len; i++) {
    printf("%s\n", sr->recvBuffer->data[i]->data);
  }
  SpeRedisPoolPut(pool, sr);
}

bool
mod_init(void) {
  pool = SpeRedisPoolCreate("127.0.0.1", "6379", 8);
  if (!pool) {
    fprintf(stderr, "SpeRedisPoolCreate Error\n");
    return false;
  }
  SpeRedis_t* sr = SpeRedisPoolGet(pool);
  if (!sr) {
    fprintf(stderr, "SpeRedisPoolGet Error\n");
    return false;
  }
  SpeRedisDo(sr, SPE_HANDLER1(on_get, sr), 2, "get", "mydokey");
  return true;
}

bool
mod_exit(void) {
  SpeRedisPoolDestroy(pool);
  return true;
}
