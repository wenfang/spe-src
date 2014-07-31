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
  if (!speServerUseAcceptMutex()) return false;
  spe_spawn(procs);
  speEpollFork();
  return true;
}

int 
main(int argc, char* argv[]) {
  if (argc > 2) {
    printf("Usage: %s [config file]\n", argv[0]);
    return 1;
  }
  // parse config file
  if ((argc==2) && !speOptCreate(argv[1])) {
    fprintf(stderr, "[ERROR] %s Parse Error\n", argv[1]);
    return 1;
  }
  // set max open files
  if (!spe_set_max_open_files(MAX_FD)) {
    fprintf(stderr, "[WARNNING] Set Max Open Files Failed\n");
  }
  // init signal
  speSignalInit();
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  // init server and monitor
  const char* ServerAddr = SpeOptString("main", "ServerAddr", "0.0.0.0");
  int ServerPort = SpeOptInt("main", "ServerPort", 0);
  if (ServerPort && !speServerInit(ServerAddr, ServerPort)) {
    fprintf(stderr, "[ERROR] speServerInit Error\n");
    return 1;
  }
  const char* MonitorAddr = SpeOptString("main", "MonitorAddr", "0.0.0.0");
  int MonitorPort = SpeOptInt("main", "MonitorPort", 0);
  if (MonitorPort && !speMonitorInit(MonitorAddr, MonitorPort)) {
    fprintf(stderr, "[ERROR] speMonitorInit Error\n");
    return 1;
  }
  // call mod_init
  if (!mod_init()) {
    fprintf(stderr, "[ERROR] mod_init Error\n");
    return 1;
  }
  speServerStart();
  speMonitorStart();
  // enter loop
  while (!GStop) {
    unsigned timeout = 300;
    if (gTaskNum || gThreadTaskNum) timeout = 0;
    speServerBeforeLoop();
    speEpollProcess(timeout);
    speServerAfterLoop();
    speTaskProcess();
    speSignalProcess();
  }
  if (!mod_exit()) {
    fprintf(stderr, "[ERROR] mod_exit Error\n");
    return 1;
  }
  speServerDeinit();
  speMonitorDeinit();
  speOptDestroy();
  return 0;
}
