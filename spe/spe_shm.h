#ifndef __SPE_SHM_H
#define __SPE_SHM_H

struct spe_shm_s {
  void*     addr;
  unsigned  size;
};
typedef struct spe_shm_s spe_shm_t;

extern spe_shm_t*
spe_shm_alloc(unsigned size);

extern void
spe_shm_free(spe_shm_t* shm);

#endif

