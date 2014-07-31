#include "spe_server.h"
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

struct spe_server_s {
  unsigned          sfd;
  speServerHandler  handler;
  SpeTask_t         listenTask;
  pthread_mutex_t*  acceptMutex;
  unsigned          acceptMutexHold;
};
typedef struct spe_server_s spe_server_t;

static spe_server_t* gServer;

static void
serverAccept() {
  int cfd = spe_sock_accept(gServer->sfd);
  if (cfd <= 0) {
    SPE_LOG_ERR("spe_sock_accept error");
    return;
  }
  if (!gServer->handler) {
    SPE_LOG_ERR("gServer no handler set");
    spe_sock_close(cfd);
    return;
  }
  spe_conn_t* conn = SpeConnCreate(cfd);
  if (!conn) {
    SPE_LOG_ERR("speConnCreate Error");
    spe_sock_close(cfd);
    return;
  }
  gServer->handler(conn);
}

/*
===================================================================================================
speServerUseAcceptMutex
===================================================================================================
*/
bool
speServerUseAcceptMutex() {
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
speServerStart
===================================================================================================
*/
void
speServerStart() {
  if (!gServer || gServer->acceptMutex) return;
  speEpollEnable(gServer->sfd, SPE_EPOLL_LISTEN, &gServer->listenTask);
}

/*
===================================================================================================
speServerBeforeLoop
===================================================================================================
*/
void
speServerBeforeLoop() {
  if (!gServer || !gServer->acceptMutex) return;
  if (!pthread_mutex_trylock(gServer->acceptMutex)) {
    if (gServer->acceptMutexHold) return;
    gServer->acceptMutexHold = 1;
    speEpollEnable(gServer->sfd, SPE_EPOLL_LISTEN, &gServer->listenTask);
  } else {
    if (gServer->acceptMutexHold) speEpollDisable(gServer->sfd, SPE_EPOLL_LISTEN);
    gServer->acceptMutexHold = 0;
  }
}

/*
===================================================================================================
speServerAfterLoop
===================================================================================================
*/
void
speServerAfterLoop() {
  if (!gServer || !gServer->acceptMutex) return;
  if (gServer->acceptMutexHold) pthread_mutex_unlock(gServer->acceptMutex);
}

/*
===================================================================================================
SpeServerSetHandler
===================================================================================================
*/
bool
SpeServerSetHandler(speServerHandler handler) {
  if (!gServer) return false;
  gServer->handler = handler;
  return true;
}

/*
===================================================================================================
SpeServerInit
===================================================================================================
*/
bool
speServerInit(const char* addr, int port) {
  if (gServer) return false;
  // create server fd
  int sfd = spe_sock_tcp_server(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_server error");
    return false;
  }
  spe_sock_set_block(sfd, 0);
  // create gServer
  gServer = calloc(1, sizeof(spe_server_t));
  if (!gServer) {
    SPE_LOG_ERR("server struct alloc error");
    spe_sock_close(sfd);
    return false;
  }
  gServer->sfd     = sfd;
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
speServerDeinit() {
  if (!gServer) return;
  if (gServer->acceptMutex) SpeShmuxDestroy(gServer->acceptMutex);
  spe_sock_close(gServer->sfd);
  free(gServer);
  gServer = NULL; 
}
