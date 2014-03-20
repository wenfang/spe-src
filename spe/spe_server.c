#include "spe_server.h"
#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void
server_accept(void* arg) {
  spe_server_t* srv = arg;
  int cfd = spe_sock_accept(srv->sfd);
  if (cfd <= 0) return;
  if (!srv->handler) {
    spe_sock_close(cfd);
    return;
  }
  srv->handler(srv, cfd);
}
/*
===================================================================================================
spe_server_disable
===================================================================================================
*/
void
spe_server_disable(spe_server_t* srv) {
  ASSERT(srv);
  spe_epoll_disable(srv->sfd, SPE_EPOLL_LISTEN);
}

/*
===================================================================================================
spe_server_enable
===================================================================================================
*/
void
spe_server_enable(spe_server_t* srv) {
  ASSERT(srv);
  spe_epoll_enable(srv->sfd, SPE_EPOLL_LISTEN, SPE_HANDLER1(server_accept, srv));
}

/*
===================================================================================================
spe_server_create
===================================================================================================
*/
spe_server_t*
spe_server_create(unsigned sfd, spe_server_conf_t* conf) {
  if (!conf) return NULL;
  spe_server_t* srv = calloc(1, sizeof(spe_server_t));
  if (!srv) {
    SPE_LOG_ERR("server create error");
    return NULL;
  }
  spe_sock_set_block(sfd, 0);
  srv->sfd = sfd;
  srv->handler = conf->handler;
  if (conf->init) conf->init(srv, conf->arg);
  if (conf->nprocs) {
    spe_spawn(conf->nprocs);
    spe_epoll_fork();
  }
  return srv;
}

/*
===================================================================================================
spe_server_destroy
===================================================================================================
*/
void
spe_server_destroy(spe_server_t* srv) {
  ASSERT(srv);
  spe_sock_close(srv->sfd);
  free(srv);
}
