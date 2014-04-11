#ifndef __SPE_SHM_H
#define __SPE_SHM_H

#include <pthread.h>

struct spe_shm_s {
  void*     addr;
  unsigned  size;
};
typedef struct spe_shm_s spe_shm_t;

extern spe_shm_t*
spe_shm_alloc(unsigned size);

extern void
spe_shm_free(spe_shm_t* shm);

extern pthread_mutex_t*
spe_shmux_create();

extern void
spe_shmux_destroy(pthread_mutex_t* shmux);

#endif

