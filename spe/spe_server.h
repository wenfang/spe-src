#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

struct spe_server_s;
typedef void (*spe_server_Init)(struct spe_server_s*, void*);
typedef void (*spe_server_Handler)(struct spe_server_s*, unsigned);

struct spe_server_conf_s {
  spe_server_Init     init;
  void*               arg;
  spe_server_Handler  handler;
  unsigned            daemon:1;
  unsigned            nprocs:7;
};
typedef struct spe_server_conf_s spe_server_conf_t;

struct spe_server_s {
  unsigned            sfd;
  spe_server_Handler  handler;
  void*               data;
};
typedef struct spe_server_s spe_server_t;

extern spe_server_t*
spe_server_create(unsigned sfd, spe_server_conf_t* conf);

extern void
spe_server_destroy(spe_server_t* srv);

extern void
spe_server_enable(spe_server_t* srv);

#endif
