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

int main(int argc, char* argv[]) {
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
  // init
  if (MainMod.init && !MainMod.init()) {
    fprintf(stderr, "[ERROR] Module Init Error ...\n");
    return 1;
  }
  if (MainMod.start && !MainMod.start()) {
    fprintf(stderr, "[ERROR] Module Start Error ...\n");
    return 1;
  }
  while (!MainMod.stop) {
    unsigned timeout = 300;
    if (g_task_num) timeout = 0;
    if (MainMod.before_loop) MainMod.before_loop();
    spe_epoll_process(timeout);
    if (MainMod.after_loop) MainMod.after_loop();
    spe_task_process();
    spe_timer_process();
    spe_signal_process();
  }
  if (MainMod.end && !MainMod.end()) {
    fprintf(stderr, "[ERROR] Module End Error ...\n");
    return 1;
  }
  if (MainMod.exit && !MainMod.exit()) {
    fprintf(stderr, "[ERROR] Module Exit Error ...\n");
    return 1;
  }
  spe_opt_destroy();
  return 0;
}
