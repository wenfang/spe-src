#include "spe_main.h"
#include "spe_server.h"
#include "spe_opt.h"
#include "spe_signal.h"
#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_util.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

bool GStop;

bool
spe_main_procs(int procs) {
  if (procs <= 1) return false;
  if (!speServerUseAcceptMutex()) return false;
  spe_spawn(procs);
  spe_epoll_fork();
  return true;
}

int 
main(int argc, char* argv[]) {
  if (argc > 2) {
    printf("Usage: %s [conf_file]\n", argv[0]);
    return 1;
  }
  // parse config file
  if ((argc==2) && !speOptCreate(argv[1])) {
    fprintf(stderr, "[ERROR] Parse File %s Error ...\n", argv[1]);
    return 1;
  }
  // set max open files
  if (!spe_set_max_open_files(MAX_FD)) {
    fprintf(stderr, "[ERROR] Set Max Open Files Failed\n");
    return 1;
  }
  // init signal
  spe_signal_init();
  spe_signal_register(SIGPIPE, SIG_IGN);
  spe_signal_register(SIGHUP, SIG_IGN);
  // call mod_init
  if (!mod_init()) {
    fprintf(stderr, "[ERROR] mod_init...\n");
    return 1;
  }
  speServerStart();
  // enter loop
  while (!GStop) {
    unsigned timeout = 300;
    if (gTaskNum) timeout = 0;
    speServerBeforeLoop();
    spe_epoll_process(timeout);
    speServerAfterLoop();
    speTaskProcess();
    spe_signal_process();
  }
  if (!mod_exit()) {
    fprintf(stderr, "[ERROR] mod_exit...\n");
    return 1;
  }
  speOptDestroy();
  return 0;
}
