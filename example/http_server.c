#include "spe.h"
#include <stdio.h>

bool
mod_init(void) {
  if (!SpeHttpServerInit("127.0.0.1", 7879)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  return true;
}

bool
mod_exit(void) {
  SpeHttpServerDeinit();
  return true;
}
