#include "spe_main.h"
#include "spe_server.h"
#include "spe_opt.h"
#include "spe_signal.h"
#include "spe_epoll.h"
#include "spe_timer.h"
#include "spe_task.h"
#include "spe_util.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

bool g_stop;

bool
spe_main_procs(int procs) {
  if (procs <= 1) return true;
  if (g_server) {
    g_server->use_accept_mutex = 1;
    g_server->accept_mutex = spe_shmux_create();
    if (!g_server->accept_mutex) {
      SPE_LOG_ERR("spe_shmux_create error");
      return false;
    }
  }
  spe_spawn(procs);
  spe_epoll_fork();
  return true;
}

int 
main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s <conf_file>\n", argv[0]);
    return 1;
  }
  // parse config file
  if (!spe_opt_create(argv[1])) {
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
  spe_server_start();
  // enter loop
  while (!g_stop) {
    unsigned timeout = 300;
    if (g_task_num) timeout = 0;
    spe_server_before_loop();
    spe_epoll_process(timeout);
    spe_server_after_loop();
    spe_task_process();
    spe_timer_process();
    spe_signal_process();
  }
  if (!mod_exit()) {
    fprintf(stderr, "[ERROR] Module Exit Error ...\n");
    return 1;
  }
  spe_opt_destroy();
  return 0;
}
