#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_shm.h"
#include "spe_task.h"


typedef void (*spe_server_Handler)(unsigned);

struct spe_server_s {
  unsigned            sfd;
  spe_server_Handler  handler;
  spe_task_t          listen_task;
  pthread_mutex_t*    accept_mutex;
  unsigned            use_accept_mutex;
  unsigned            accept_mutex_hold;
};
typedef struct spe_server_s spe_server_t;

extern spe_server_t* g_server;

extern void
spe_server_start();

extern void
spe_server_before_loop();

extern void
spe_server_after_loop();

extern bool
spe_server_init(const char* addr, int port, spe_server_Handler handler);

extern void
spe_server_deinit();

#endif
