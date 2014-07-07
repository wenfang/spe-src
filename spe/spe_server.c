#include "spe_server.h"
#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

spe_server_t* g_server;

static void
server_accept() {
  int cfd = spe_sock_accept(g_server->sfd);
  if (cfd <= 0) return;
  if (!g_server->handler) {
    spe_sock_close(cfd);
    return;
  }
  g_server->handler(cfd);
}

/*
===================================================================================================
spe_server_start
===================================================================================================
*/
void
spe_server_start() {
  if (!g_server || g_server->use_accept_mutex) return;
  spe_epoll_enable(g_server->sfd, SPE_EPOLL_LISTEN, &g_server->listen_task);
}

/*
===================================================================================================
spe_server_before_loop
===================================================================================================
*/
void
spe_server_before_loop() {
  if (!g_server || !g_server->use_accept_mutex) return;
  if (!pthread_mutex_trylock(g_server->accept_mutex)) {
    if (g_server->accept_mutex_hold) return;
    g_server->accept_mutex_hold = 1;
    spe_epoll_enable(g_server->sfd, SPE_EPOLL_LISTEN, &g_server->listen_task);
  } else {
    if (g_server->accept_mutex_hold) spe_epoll_disable(g_server->sfd, SPE_EPOLL_LISTEN);
    g_server->accept_mutex_hold = 0;
  }
}

/*
===================================================================================================
spe_server_after_loop
===================================================================================================
*/
void
spe_server_after_loop() {
  if (!g_server || !g_server->use_accept_mutex) return;
  if (g_server->accept_mutex_hold) pthread_mutex_unlock(g_server->accept_mutex);
}

/*
===================================================================================================
spe_server_init
===================================================================================================
*/
bool
spe_server_init(const char* addr, int port, spe_server_Handler handler) {
  int sfd = spe_sock_tcp_server(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_server error");
    return false;
  }
  g_server = calloc(1, sizeof(spe_server_t));
  if (!g_server) {
    SPE_LOG_ERR("server struct alloc error");
    spe_sock_close(sfd);
    return false;
  }
  spe_sock_set_block(sfd, 0);
  g_server->sfd      = sfd;
  g_server->handler  = handler;
  spe_task_init(&g_server->listen_task);
  g_server->listen_task.handler = SPE_HANDLER0(server_accept);
  return true;
}

/*
===================================================================================================
spe_server_deinit
===================================================================================================
*/
void
spe_server_deinit() {
  if (!g_server) return;
  if (g_server->use_accept_mutex) spe_shmux_destroy(g_server->accept_mutex);
  spe_sock_close(g_server->sfd);
}
