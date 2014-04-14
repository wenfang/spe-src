#include "mjvec.h"
#include <stdlib.h>

/*
===============================================================================
mjvec_ready
  mjvec ready need_size
===============================================================================
*/
static bool mjvec_ready(mjvec vec, unsigned int need_size) {
  // vec is enough
  unsigned int total = vec->_total;
  if (need_size <= total) return true;
  // realloc space
  vec->_total = 30 + need_size + (need_size >> 3);
  void** new_obj = (void**) realloc(vec->_obj, vec->_total * sizeof(void*));
  if (!new_obj) {
    vec->_total = total;
    return false;
  }
  // copy new_ndata
  vec->_obj = new_obj;
  for (int i = vec->len; i < vec->_total; i++) vec->_obj[i] = NULL; 
  return true;
}

/*
===============================================================================
mjvec_readplus
===============================================================================
*/
static inline bool mjvec_readyplus(mjvec vec, unsigned int n) {
  return mjvec_ready(vec, vec->len + n);
}

bool mjvec_add(mjvec vec, void* value) {
  if (!vec || !value) return false;
  if (!mjvec_readyplus(vec, 1)) return false;
  vec->_obj[vec->len] = value;
  vec->len++;
  return true;
}

void* mjvec_get(mjvec vec, unsigned int idx) {
  if (!vec || idx >= vec->len) return NULL;
  return vec->_obj[idx];
}

mjvec mjvec_new(mjProc obj_free) {
  mjvec vec = (mjvec) calloc(1, sizeof(struct mjvec));
  if (!vec) return NULL;
  vec->_obj_free = obj_free;
  return vec;
}

bool mjvec_delete(mjvec vec) {
  if (!vec) return false;
  for (int i = 0; i < vec->len; i++) {
    if (vec->_obj_free && vec->_obj[i]) vec->_obj_free(vec->_obj[i]);
  }
  free(vec->_obj);
  free(vec);
  return true;
}
