#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_shm.h"
#include "spe_task.h"
#include "spe_epoll.h"

typedef void (*spe_server_Handler)(unsigned);

struct spe_server_s {
  unsigned            _sfd;
  spe_server_Handler  _handler;
  spe_task_t          _listen_task;
  pthread_mutex_t*    accept_mutex;
  unsigned            accept_mutex_hold;
};
typedef struct spe_server_s spe_server_t;

extern spe_server_t* g_server;

static inline void
spe_server_start() {
  if (!g_server || g_server->accept_mutex) return;
  spe_epoll_enable(g_server->_sfd, SPE_EPOLL_LISTEN, &g_server->_listen_task);
}

static inline void
spe_server_before_loop() {
  if (!g_server || !g_server->accept_mutex) return;
  if (!pthread_mutex_trylock(g_server->accept_mutex)) {
    if (g_server->accept_mutex_hold) return;
    g_server->accept_mutex_hold = 1;
    spe_epoll_enable(g_server->_sfd, SPE_EPOLL_LISTEN, &g_server->_listen_task);
  } else {
    if (g_server->accept_mutex_hold) spe_epoll_disable(g_server->_sfd, SPE_EPOLL_LISTEN);
    g_server->accept_mutex_hold = 0;
  }
}

static inline void
spe_server_after_loop() {
  if (!g_server || !g_server->accept_mutex) return;
  if (g_server->accept_mutex_hold) pthread_mutex_unlock(g_server->accept_mutex);
}

// user call spe_server_init and spe_server_deinit
extern bool
spe_server_init(const char* addr, int port, spe_server_Handler handler);

extern void
spe_server_deinit();

#endif
