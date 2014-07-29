#include "spe_monitor.h"
#include "spe_log.h"
#include "spe_sock.h"
#include <stdlib.h>

struct speMonitor_s {
  unsigned  mfd;
};
typedef struct speMonitor_s speMonitor_t;

static speMonitor_t* gMonitor;

/*
===================================================================================================
SpeMonitorInit
===================================================================================================
*/
bool
SpeMonitorInit(const char* addr, int port) {
  if (gMonitor) return false;
  int mfd = spe_sock_tcp_server(addr, port);
  if (mfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_server error");
    return false;
  }
  spe_sock_set_block(mfd, 0);
  gMonitor = calloc(1, sizeof(speMonitor_t));
  if (!gMonitor) {
    SPE_LOG_ERR("monitor struct alloc error");
    spe_sock_close(mfd);
    return false;
  }
  gMonitor->mfd = mfd;
  return true;
}

/*
===================================================================================================
SpeMonitorDeInit
===================================================================================================
*/
void
SpeMonitorDeInit() {
  if (!gMonitor) return;
  spe_sock_close(gMonitor->mfd);
  free(gMonitor);
  gMonitor = NULL;
}
