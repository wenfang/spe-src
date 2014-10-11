#include "spe_main.h"
#include "spe_server.h"
#include "spe_monitor.h"
#include "spe_opt.h"
#include "spe_signal.h"
#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_util.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

bool GStop;

/*
===================================================================================================
SpeProcs
===================================================================================================
*/
bool
SpeProcs(int procs) {
  if (procs <= 1) return false;
  if (!serverUseAcceptMutex()) return false;
  spe_spawn(procs);
  epollFork();
  return true;
}

int 
main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stdout, "Usage: %s [configFile]\n", argv[0]);
    return 1;
  }
  // parse config file
  if ((argc==2) && !speOptCreate(argv[1])) {
    fprintf(stderr, "[ERROR] %s Parse Error\n", argv[1]);
    return 1;
  }
  // set max open files
  if (!speSetMaxOpenFiles(MAX_FD)) {
    fprintf(stderr, "[WARNNING] Set Max Open Files Failed\n");
  }
  // init signal
  speSignalInit();
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  // check mode
  const char* mode = SpeOptString(NULL, "Mode", "Server");
  if (!strcmp(mode, "Server")) {
    if (!speServerInit()) {
      fprintf(stderr, "[ERROR] server init error");
      return 1;
    }
  }
  // call mod_init
  if (!mod_init()) {
    fprintf(stderr, "[ERROR] mod_init Error\n");
    return 1;
  }
  serverEnable();
  monitorEnable();
  // enter loop
  while (!GStop) {
    unsigned timeout = 300;
    if (gTaskNum || gThreadTaskNum) timeout = 0;
    serverBeforeLoop();
    epollProcess(timeout);
    serverAfterLoop();
    taskProcess();
    speSignalProcess();
  }
  if (!mod_exit()) {
    fprintf(stderr, "[ERROR] mod_exit Error\n");
    return 1;
  }
  speOptDestroy();
  return 0;
}
