#include "spe_shm.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdlib.h>
#include <sys/mman.h>

spe_shm_t*
spe_shm_alloc(unsigned size) {
  spe_shm_t* shm = calloc(1, sizeof(spe_shm_t));
  if (!shm) {
    SPE_LOG_ERR("spe_shm alloc calloc error");
    return NULL;
  }

  shm->addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (!shm->addr) {
    SPE_LOG_ERR("spe shm alloc mmap error");
    free(shm);
    return NULL; 
  }
  shm->size = size;
  return shm;
}

void
spe_shm_free(spe_shm_t* shm) {
  ASSERT(shm);
  if (munmap(shm->addr, shm->size) == -1) {
    SPE_LOG_ERR("spe_shm_free error");
  }
}
