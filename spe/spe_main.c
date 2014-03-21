#include "spe_main.h"
#include "spe_opt.h"
#include "spe_signal.h"
#include "spe_module.h"
#include "spe_epoll.h"
#include "spe_timer.h"
#include "spe_task.h"
#include "spe_util.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

bool spe_main_stop;

int init(void) {
  if (!spe_set_max_open_files(MAX_FD)) {
    fprintf(stderr, "[ERROR] Set Max Open Files Failed\n");
    return 1;
  }
  // init signal
  spe_signal_init();
  spe_signal_register(SIGPIPE, SIG_IGN);
  spe_signal_register(SIGHUP, SIG_IGN);
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s <conf_file>\n", argv[0]);
    return 1;
  }
  // parse config file
  if (!spe_opt_create(argv[1])) {
    fprintf(stderr, "[ERROR] Parse File %s.\n", argv[1]);
    return 1;
  }
  // init
  int res = init();
  if (res) return res;
  // run module init
  if (!spe_modules_init()) {
    fprintf(stderr, "[ERROR] Module Init\n");
    return 1;
  }
  // run module start
  if (!spe_modules_start()) {
    fprintf(stderr, "[ERROR] Module Start\n");
    return 1;
  }
  while (!spe_main_stop) {
    unsigned timeout = 500;
    if (!spe_task_empty()) timeout = 0;
    spe_epoll_process(timeout);
    spe_task_process();
    spe_timer_process();
    spe_signal_process();
  }
  if (!spe_modules_end()) {
    fprintf(stderr, "[ERROR] Module End\n");
    return 1;
  }
  if (!spe_modules_exit()) {
    fprintf(stderr, "[ERROR] Module Exit\n");
    return 1;
  }
  spe_opt_destroy();
  return 0;
}
