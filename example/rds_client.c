#include "spe.h"
#include <stdio.h>

static void
on_get(void* arg) {
  spe_redis_t* sr = arg;
  if (sr->error) {
    fprintf(stderr, "spe_redis_t error\n");
    g_stop = true;
    return;
  }
  for (int i=0; i<sr->recv_buffer->len; i++) {
    printf("%s\n", sr->recv_buffer->data[i]->data);
  }
  spe_redis_destroy(sr);
  g_stop = true;
}

/*
static void
on_set(void* arg) {
  spe_redis_t* sr = arg;
  if (sr->error) {
    fprintf(stderr, "spe_redis_t error\n");
    g_stop = true;
    return;
  }
  fprintf(stderr, "set ok\n");
  g_stop = true;
}
*/

bool
mod_init(void) {
  spe_redis_t* sr = spe_redis_create("127.0.0.1", "6379");
  if (!sr) {
    fprintf(stderr, "spe_redis create error\n");
    return false;
  }
  spe_redis_do(sr, SPE_HANDLER1(on_get, sr), 2, "get", "mydokey");
  return true;
}

bool
mod_exit(void) {
  return true;
}
