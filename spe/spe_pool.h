#ifndef __SPE_POOL_H
#define __SPE_POOL_H

typedef void (*spe_pool_Free)(void*);

struct spe_pool_s {
  unsigned      total;
  unsigned      len;
  spe_pool_Free handler;
  void**        data;
};
typedef struct spe_pool_s spe_pool_t;

extern void*  
spe_pool_get(spe_pool_t* pool);

extern void
spe_pool_put(spe_pool_t* pool, void* obj);

extern spe_pool_t*
spe_pool_create(unsigned total, spe_pool_Free handler);

extern void
spe_pool_destroy(spe_pool_t* pool);

#endif
