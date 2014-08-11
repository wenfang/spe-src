#include "spe.h"
#include <stdio.h>

static void
test1Handler(SpeHttpRequest_t* request) {
  SpeConnWrites(request->conn, "test1 handler called\r\n");
  SpeHttpRequestFinish(request);
}

static void
test2Handler(SpeHttpRequest_t* request) {
  SpeConnWrites(request->conn, "test2 handler called\r\n");
  SpeHttpRequestFinish(request);
}

bool
mod_init(void) {
  if (!SpeHttpServerInit("127.0.0.1", 7879)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  SpeHttpRegisterHandler("/test1", test1Handler);
  SpeHttpRegisterHandler("/test2", test2Handler);
  return true;
}

bool
mod_exit(void) {
  SpeHttpServerDeinit();
  SpeHttpUnregisterHandler();
  return true;
}
