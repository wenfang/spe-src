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
  unsigned          _sfd;
  speServerHandler  _handler;
  SpeTask_t         _listen_task;
  pthread_mutex_t*  _accept_mutex;
  unsigned          _accept_mutex_hold;
};
typedef struct spe_server_s spe_server_t;

static spe_server_t* gServer;

static void
serverAccept() {
  int cfd = spe_sock_accept(gServer->_sfd);
  if (cfd <= 0) {
    SPE_LOG_ERR("spe_sock_accept error");
    return;
  }
  if (!gServer->_handler) {
    SPE_LOG_ERR("gServer no handler set");
    spe_sock_close(cfd);
    return;
  }
  gServer->_handler(cfd);
}

/*
===================================================================================================
speServerUseAcceptMutex
===================================================================================================
*/
bool
speServerUseAcceptMutex() {
  if (!gServer) return false;
  if (gServer->_accept_mutex) return true;
  gServer->_accept_mutex = spe_shmux_create();
  if (!gServer->_accept_mutex) {
    SPE_LOG_ERR("spe_shmux_create error");
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
  if (!gServer || gServer->_accept_mutex) return;
  spe_epoll_enable(gServer->_sfd, SPE_EPOLL_LISTEN, &gServer->_listen_task);
}

/*
===================================================================================================
speServerBeforeLoop
===================================================================================================
*/
void
speServerBeforeLoop() {
  if (!gServer || !gServer->_accept_mutex) return;
  if (!pthread_mutex_trylock(gServer->_accept_mutex)) {
    if (gServer->_accept_mutex_hold) return;
    gServer->_accept_mutex_hold = 1;
    spe_epoll_enable(gServer->_sfd, SPE_EPOLL_LISTEN, &gServer->_listen_task);
  } else {
    if (gServer->_accept_mutex_hold) spe_epoll_disable(gServer->_sfd, SPE_EPOLL_LISTEN);
    gServer->_accept_mutex_hold = 0;
  }
}

/*
===================================================================================================
speServerAfterLoop
===================================================================================================
*/
void
speServerAfterLoop() {
  if (!gServer || !gServer->_accept_mutex) return;
  if (gServer->_accept_mutex_hold) pthread_mutex_unlock(gServer->_accept_mutex);
}

/*
===================================================================================================
SpeServerInit
===================================================================================================
*/
bool
SpeServerInit(const char* addr, int port, speServerHandler handler) {
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
  gServer->_sfd     = sfd;
  gServer->_handler = handler;
  SpeTaskInit(&gServer->_listen_task);
  gServer->_listen_task.Handler = SPE_HANDLER0(serverAccept);
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
  if (gServer->_accept_mutex) spe_shmux_destroy(gServer->_accept_mutex);
  spe_sock_close(gServer->_sfd);
  free(gServer);
  gServer = NULL; 
}
