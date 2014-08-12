#include "spe_monitor.h"
#include "spe_log.h"
#include "spe_sock.h"
#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_conn.h"
#include <stdlib.h>

struct speMonitor_s {
  unsigned  mfd;
  SpeTask_t monitorTask;
};
typedef struct speMonitor_s speMonitor_t;

static speMonitor_t* gMonitor;

#define MONITOR_WAITCMD 1
#define MONITOR_RESULT  2

struct monitorConn_s {
  SpeConn_t*  conn;
  unsigned    status;
};
typedef struct monitorConn_s monitorConn_t;

static void
processCmd(monitorConn_t* mconn) {
  SpeConn_t* conn = mconn->conn;
  if (!strncasecmp(conn->Buffer->data, "quit", 4)) {
    SpeConnDestroy(conn);
    free(mconn);
    return;
  }
  SpeConnWrites(conn, "Stat Info ~~~\r\n");
  SpeConnFlush(conn);
}

static void
driver_machine(void* arg) {
  monitorConn_t* mconn = arg;
  SpeConn_t* conn = mconn->conn;
  if (conn->Closed || conn->Error) {
    SpeConnDestroy(conn);
    free(mconn);
    return;
  }

  switch (mconn->status) {
    case MONITOR_WAITCMD:
      mconn->status = MONITOR_RESULT;
      processCmd(mconn);
      break;
    case MONITOR_RESULT:
      mconn->status = MONITOR_WAITCMD;
      SpeConnReaduntil(conn, "\r\n");
      break;
  }
}

static void
monitorAccept() {
  int cfd = spe_sock_accept(gMonitor->mfd);
  if (cfd < 0) {
    SPE_LOG_ERR("monitor sock accept error");
    return;
  }
  monitorConn_t* mconn = calloc(1, sizeof(monitorConn_t));
  if (!mconn) {
    SPE_LOG_ERR("monitorConn_t calloc error");
    SpeSockClose(cfd);
    return;
  }
  mconn->conn = SpeConnCreate(cfd);
  if (!mconn->conn) {
    SPE_LOG_ERR("monitor SpeConnCreate error");
    free(mconn);
    SpeSockClose(cfd);
    return;
  }
  mconn->conn->ReadCallback.Handler   = SPE_HANDLER1(driver_machine, mconn);
  mconn->conn->WriteCallback.Handler  = SPE_HANDLER1(driver_machine, mconn);
  mconn->status = MONITOR_WAITCMD;
  SpeConnReaduntil(mconn->conn, "\r\n");
}

/*
===================================================================================================
monitorEnable
===================================================================================================
*/
void
monitorEnable() {
  if (!gMonitor) return;
  epollEnable(gMonitor->mfd, SPE_EPOLL_LISTEN, &gMonitor->monitorTask);
}

/*
===================================================================================================
SpeMonitorRegister
===================================================================================================
*/
bool
SpeMonitorRegister(const char* name, int type, void* value) {
  return true;
}

/*
===================================================================================================
SpeMonitorInit
===================================================================================================
*/
bool
SpeMonitorInit(const char* addr, int port) {
  if (gMonitor) return false;
  int mfd = SpeSockTcpServer(addr, port);
  if (mfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error");
    return false;
  }
  SpeSockSetBlock(mfd, 0);
  gMonitor = calloc(1, sizeof(speMonitor_t));
  if (!gMonitor) {
    SPE_LOG_ERR("monitor struct alloc error");
    SpeSockClose(mfd);
    return false;
  }
  gMonitor->mfd = mfd;
  SpeTaskInit(&gMonitor->monitorTask);
  gMonitor->monitorTask.Handler = SPE_HANDLER0(monitorAccept);
  return true;
}

/*
===================================================================================================
SpeMonitorDeInit
===================================================================================================
*/
void
SpeMonitorDeinit() {
  if (!gMonitor) return;
  SpeSockClose(gMonitor->mfd);
  free(gMonitor);
  gMonitor = NULL;
}
