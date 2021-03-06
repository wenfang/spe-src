#include "spe_server.h"
#include "spe_opt.h"
#include "spe_task.h"
#include "spe_shm.h"
#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct speServer_s {
  unsigned          sfd;
  SpeServerHandler  handler;
  SpeTask_t         listenTask;
  pthread_mutex_t*  acceptMutex;
  unsigned          acceptMutexHold;
};
typedef struct speServer_s speServer_t;

// global server
static speServer_t* gServer;

static void
serverAccept() {
  int cfd = SpeSockAccept(gServer->sfd);
  if (cfd <= 0) return;
  if (!gServer->handler) {
    SPE_LOG_ERR("gServer no handler set");
    SpeSockClose(cfd);
    return;
  }
  SpeConn_t* conn = SpeConnCreate(cfd);
  if (!conn) {
    SPE_LOG_ERR("SpeConnCreate Error");
    SpeSockClose(cfd);
    return;
  }
  gServer->handler(conn);
}

/*
===================================================================================================
serverUseAcceptMutex
===================================================================================================
*/
bool
serverUseAcceptMutex() {
  if (!gServer) return false;
  if (gServer->acceptMutex) return true;
  gServer->acceptMutex = SpeShmuxCreate();
  if (!gServer->acceptMutex) {
    SPE_LOG_ERR("SpeShmuxCreate error");
    return false;
  }
  return true;
}

/*
===================================================================================================
serverEnable
===================================================================================================
*/
void
serverEnable() {
  if (!gServer || gServer->acceptMutex) return;
  epollEnable(gServer->sfd, SPE_EPOLL_LISTEN, &gServer->listenTask);
}

/*
===================================================================================================
serverBeforeLoop
===================================================================================================
*/
void
serverBeforeLoop() {
  if (!gServer || !gServer->acceptMutex) return;
  if (!pthread_mutex_trylock(gServer->acceptMutex)) {
    if (gServer->acceptMutexHold) return;
    gServer->acceptMutexHold = 1;
    epollEnable(gServer->sfd, SPE_EPOLL_LISTEN, &gServer->listenTask);
  } else {
    if (gServer->acceptMutexHold) epollDisable(gServer->sfd, SPE_EPOLL_LISTEN);
    gServer->acceptMutexHold = 0;
  }
}

/*
===================================================================================================
serverAfterLoop
===================================================================================================
*/
void
serverAfterLoop() {
  if (!gServer || !gServer->acceptMutex) return;
  if (gServer->acceptMutexHold) pthread_mutex_unlock(gServer->acceptMutex);
}


/*
===================================================================================================
SpeServerInit
===================================================================================================
*/
bool
speServerInit() {
  if (gServer) return false;
  const char* addr = SpeOptString(NULL, "ServerAddr", "127.0.0.1");
  int port = SpeOptInt(NULL, "ServerPort", 7879);
  int sfd = SpeSockTcpServer(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error");
    return false;
  }
  SpeSockSetBlock(sfd, 0);
  // create gServer
  gServer = calloc(1, sizeof(speServer_t));
  if (!gServer) {
    SPE_LOG_ERR("server struct alloc error");
    SpeSockClose(sfd);
    return false;
  }
  gServer->sfd      = sfd;
  gServer->handler  = NULL;
  SpeTaskInit(&gServer->listenTask);
  gServer->listenTask.Handler = SPE_HANDLER0(serverAccept);
  return true;
}

/*
===================================================================================================
SpeServerInit
===================================================================================================
*/
bool
SpeServerInit(const char* addr, int port, SpeServerHandler handler) {
  // server created? return false
  if (gServer) return false;
  // create server fd
  int sfd = SpeSockTcpServer(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error");
    return false;
  }
  SpeSockSetBlock(sfd, 0);
  // create gServer
  gServer = calloc(1, sizeof(speServer_t));
  if (!gServer) {
    SPE_LOG_ERR("server struct alloc error");
    SpeSockClose(sfd);
    return false;
  }
  gServer->sfd      = sfd;
  gServer->handler  = handler;
  SpeTaskInit(&gServer->listenTask);
  gServer->listenTask.Handler = SPE_HANDLER0(serverAccept);
  return true;
}

/*
===================================================================================================
SpeServerDeinit
===================================================================================================
*/
void
SpeServerDeinit() {
  if (!gServer) return;
  if (gServer->acceptMutex) SpeShmuxDestroy(gServer->acceptMutex);
  SpeSockClose(gServer->sfd);
  free(gServer);
  gServer = NULL; 
}
