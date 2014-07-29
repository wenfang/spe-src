#include "spe.h"
#include <stdio.h>

void
process(spe_string_t* input) {
  fprintf(stdout, "You Input is: %s\r\n", input->data);
  fflush(stdout);
}

bool
mod_init(void) {
  SpeIo_t* Stdin = SpeIoCreateFd(0);
  /*
  while (1) {
    SpeIoReaduntil(Stdin, "\r\n");
    if (Stdin->Buffer->len == 0) break;
    process(Stdin->Buffer);
  }
  */
  SpeIoReaduntil(Stdin, "\r\n\r\n");
  fprintf(stdout, "Bye!!!\r\n");
  fflush(stdout);
  SpeIoDestroy(Stdin);
  GStop = true;
  return true;
}

bool
mod_exit(void) {
  return true;
}
