#include "spe.h"
#include <stdio.h>

static void
test(void* arg) {
  printf("test run %ld\n", (long)arg);
}

bool
mod_init() {
  SpeTpoolInit();
  for (int i=0; i<300; i++) {
    SpeTpoolDo(SPE_HANDLER1(test, (void*)(long)i));
  }
  return true;
}

bool
mod_exit() {
  SpeTpoolDeinit();
  return true;
}
