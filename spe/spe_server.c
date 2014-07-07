#include "spe_server.h"
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
  int cfd = spe_sock_accept(g_server->_sfd);
  if (cfd <= 0) return;
  if (!g_server->_handler) {
    spe_sock_close(cfd);
    return;
  }
  g_server->_handler(cfd);
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
  g_server->_sfd      = sfd;
  g_server->_handler  = handler;
  spe_task_init(&g_server->_listen_task);
  g_server->_listen_task.handler = SPE_HANDLER0(server_accept);
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
  if (g_server->accept_mutex) {
    spe_shmux_destroy(g_server->accept_mutex);
    g_server->accept_mutex = NULL;
  }
  spe_sock_close(g_server->_sfd);
  g_server = NULL; 
}
