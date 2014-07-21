#include "spe.h"
#include <stdio.h>

static void
test(void) {
  printf("test run\n");
}

bool
mod_init() {
  SpeTpoolInit();
  for (int i=0; i<10; i++) {
    SpeTpoolDo(SPE_HANDLER0(test));
  }
  return true;
}

bool
mod_exit() {
  SpeTpoolDeinit();
  return true;
}
