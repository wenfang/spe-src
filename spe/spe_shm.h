#ifndef __SPE_SHM_H
#define __SPE_SHM_H

#include <pthread.h>

struct spe_shm_s {
  void*     addr;
  unsigned  size;
};
typedef struct spe_shm_s spe_shm_t;

extern spe_shm_t*
SpeShmAlloc(unsigned size);

extern void
SpeShmFree(spe_shm_t* shm);

extern pthread_mutex_t*
SpeShmuxCreate();

extern void
SpeShmuxDestroy(pthread_mutex_t* shmux);

#endif
