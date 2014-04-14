#include "spe.h"
#include <stdio.h>

static void
on_get(void* arg) {
  spe_redis_t* sr = arg;
  if (sr->error) {
    fprintf(stderr, "spe_redis_t error\n");
    spe_main_stop = 1;
    return;
  }
  for (int i=0; i<sr->recv_buffer->len; i++) {
    printf("%s\n", sr->recv_buffer->data[i]->data);
  }
  spe_main_stop = 1;
}

static void
on_set(void* arg) {
  spe_redis_t* sr = arg;
  if (sr->error) {
    fprintf(stderr, "spe_redis_t error\n");
    spe_main_stop = 1;
    return;
  }
  fprintf(stderr, "set ok\n");
  spe_main_stop = 1;
}

static bool
test_start(void) {
  spe_redis_t* sr = spe_redis_create("127.0.0.1", "6379");
  if (!sr) {
    fprintf(stderr, "spe_redis create error\n");
    return false;
  }
  spe_redis_do(sr, SPE_HANDLER1(on_get, sr), 2, "get", "mydokey");
  return true;
}

static spe_module_t test_module = {
  NULL,
  NULL,
  test_start,
  NULL,
};

__attribute__((constructor))
static void
__test_init(void) {
  spe_register_module(&test_module);
}
