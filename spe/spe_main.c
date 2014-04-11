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

bool spe_main_stop;
static spe_module_t* MainMod;

void
spe_register_module(spe_module_t* mod) {
  ASSERT(mod);
  MainMod = mod;
}

static int init(void) {
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
  if (!MainMod) {
    fprintf(stderr, "No Module Set ... \n");
    return 1;
  }
  // parse config file
  if (!spe_opt_create(argv[1])) {
    fprintf(stderr, "Parse File %s Error ...\n", argv[1]);
    return 1;
  }
  // init
  int res = init();
  if (res) return res;
  if (MainMod->init) {
    if (!MainMod->init()) {
      fprintf(stderr, "Module Init Error ...\n");
      return 1;
    }
  }
  if (MainMod->start) {
    if (!MainMod->start()) {
      fprintf(stderr, "Module Start Error ...\n");
      return 1;
    }
  }
  while (!spe_main_stop) {
    unsigned timeout = 500;
    if (g_task_num) timeout = 0;
    if (MainMod->before_loop) MainMod->before_loop();
    spe_epoll_process(timeout);
    if (MainMod->after_loop) MainMod->after_loop();
    spe_task_process();
    spe_timer_process();
    spe_signal_process();
  }
  if (MainMod->end) {
    if (!MainMod->end()) {
      fprintf(stderr, "Module End Error ...\n");
      return 1;
    }
  }
  if (MainMod->exit) {
    if (!MainMod->exit()) {
      fprintf(stderr, "Module Exit Error ...\n");
      return 1;
    }
  }
  spe_opt_destroy();
  return 0;
}
