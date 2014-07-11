#include "spe_server.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct spe_server_s {
  unsigned            _sfd;
  spe_server_Handler  _handler;
  spe_task_t          _listen_task;
  pthread_mutex_t*    _accept_mutex;
  unsigned            _accept_mutex_hold;
};
typedef struct spe_server_s spe_server_t;

static spe_server_t* g_server;

static void
server_accept() {
  int cfd = spe_sock_accept(g_server->_sfd);
  if (cfd <= 0) {
    SPE_LOG_ERR("spe_sock_accept error");
    return;
  }
  if (!g_server->_handler) {
    SPE_LOG_ERR("g_server no handler set");
    spe_sock_close(cfd);
    return;
  }
  g_server->_handler(cfd);
}

/*
===================================================================================================
speServerUseAcceptMutex
===================================================================================================
*/
bool
speServerUseAcceptMutex() {
  if (!g_server) return false;
  if (g_server->_accept_mutex) return true;
  g_server->_accept_mutex = spe_shmux_create();
  if (!g_server->_accept_mutex) {
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
  if (!g_server || g_server->_accept_mutex) return;
  spe_epoll_enable(g_server->_sfd, SPE_EPOLL_LISTEN, &g_server->_listen_task);
}

/*
===================================================================================================
speServerBeforeLoop
===================================================================================================
*/
void
speServerBeforeLoop() {
  if (!g_server || !g_server->_accept_mutex) return;
  if (!pthread_mutex_trylock(g_server->_accept_mutex)) {
    if (g_server->_accept_mutex_hold) return;
    g_server->_accept_mutex_hold = 1;
    spe_epoll_enable(g_server->_sfd, SPE_EPOLL_LISTEN, &g_server->_listen_task);
  } else {
    if (g_server->_accept_mutex_hold) spe_epoll_disable(g_server->_sfd, SPE_EPOLL_LISTEN);
    g_server->_accept_mutex_hold = 0;
  }
}

/*
===================================================================================================
speServerAfterLoop
===================================================================================================
*/
void
speServerAfterLoop() {
  if (!g_server || !g_server->_accept_mutex) return;
  if (g_server->_accept_mutex_hold) pthread_mutex_unlock(g_server->_accept_mutex);
}

/*
===================================================================================================
SpeServerInit
===================================================================================================
*/
bool
SpeServerInit(const char* addr, int port, spe_server_Handler handler) {
  if (g_server) return false;
  // create server fd
  int sfd = spe_sock_tcp_server(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_server error");
    return false;
  }
  spe_sock_set_block(sfd, 0);
  // create g_server
  g_server = calloc(1, sizeof(spe_server_t));
  if (!g_server) {
    SPE_LOG_ERR("server struct alloc error");
    spe_sock_close(sfd);
    return false;
  }
  g_server->_sfd      = sfd;
  g_server->_handler  = handler;
  spe_task_init(&g_server->_listen_task);
  g_server->_listen_task.handler = SPE_HANDLER0(server_accept);
  return true;
}

/*
===================================================================================================
SpeServerDeinit
===================================================================================================
*/
void
SpeServerDeinit() {
  if (!g_server) return;
  if (g_server->_accept_mutex) spe_shmux_destroy(g_server->_accept_mutex);
  spe_sock_close(g_server->_sfd);
  free(g_server);
  g_server = NULL; 
}
