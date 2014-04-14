#include "spe_pool.h"
#include "spe_util.h"
#include <stdlib.h>

/*
===================================================================================================
spe_pool_get
===================================================================================================
*/
void*
spe_pool_get(spe_pool_t* pool) {
  ASSERT(pool);
  if (pool->len <= 0) return NULL;
  return pool->data[pool->len--];
}

/*
===================================================================================================
spe_pool_put
===================================================================================================
*/
void
spe_pool_put(spe_pool_t* pool, void* obj) {
  ASSERT(pool && obj);
  pool->data[pool->len++] = obj;
}

/*
===================================================================================================
spe_pool_create
===================================================================================================
*/
spe_pool_t*
spe_pool_create(unsigned total, spe_pool_Free handler) {
  spe_pool_t* pool = calloc(1, sizeof(spe_pool_t));
  if (!pool) {
    SPE_LOG_ERR("calloc spe_pool_t error");
    return NULL;
  }
  pool->total = total;
  pool->data = calloc(pool->total, sizeof(void*));
  if (!pool->data) {
    SPE_LOG_ERR("pool data alloc error");
    free(pool);
    return NULL; 
  }
  return pool;
}

/*
===================================================================================================
spe_pool_destroy
===================================================================================================
*/
void
spe_pool_destroy(spe_pool_t* pool) {
  ASSERT(pool);
  for (int i=0; i<pool->len; i++) {
    if (pool->handler) pool->handler(pool->data[i]);
  }
  free(pool->data);
  free(pool);
}
