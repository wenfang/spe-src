#include "spe_redis.h"
#include "spe_log.h"

spe_redis_t*
spe_redis_create(const char* host, const char* port) {
  spe_redis_t* sr = calloc(1, sizeof(spe_redis_t));
  if (!sr) {
    SPE_LOG_ERR("spe_redis calloc error");
    return NULL;
  }
  return sr;
}
