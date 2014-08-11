#include "spe.h"
#include <stdio.h>

static void
testHandler(SpeHttpRequest_t* request) {
  SpeConnWrites(request->conn, "test handler called\r\n");
  SpeHttpRequestFinish(request);
}

bool
mod_init(void) {
  if (!SpeHttpServerInit("127.0.0.1", 7879)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  SpeHttpRegisterHandler("/test", testHandler);
  return true;
}

bool
mod_exit(void) {
  SpeHttpServerDeinit();
  SpeHttpUnregisterHandler();
  return true;
}
