#include "spe.h"
#include <stdio.h>

#define PF_INIT 0
#define PF_END  1

struct pf_conn_s {
  spe_conn_t* conn;
  int         status;
};
typedef struct pf_conn_s pf_conn_t;

static spe_server_t* srv;

static void
driver_machine(void* arg) {
  pf_conn_t* pf_conn = arg;
  spe_conn_t* conn = pf_conn->conn;
  if (conn->closed || conn->error) {
    spe_conn_destroy(conn);
    free(pf_conn);
    return;
  }
  switch (pf_conn->status) {
    case PF_INIT:
      pf_conn->status = PF_END;
      spe_conn_writes(conn, "OK\r\n");
      spe_conn_flush(conn);
      break;
    case PF_END:
      spe_conn_destroy(conn);
      free(pf_conn);
      break;
  }
}

static void
run(spe_server_t* srv, unsigned cfd) {
  spe_conn_t* conn = spe_conn_create(cfd);
  if (!conn) {
    spe_sock_close(cfd);
    return;
  }
  pf_conn_t* pf_conn = calloc(1, sizeof(pf_conn_t));
  if (!pf_conn) {
    spe_conn_destroy(conn);
    return;
  }
  conn->read_callback_task.handler  = SPE_HANDLER1(driver_machine, pf_conn);
  conn->write_callback_task.handler = SPE_HANDLER1(driver_machine, pf_conn);
  pf_conn->conn   = conn;
  pf_conn->status = PF_INIT;
  spe_conn_readuntil(pf_conn->conn, "\r\n\r\n");
}

static spe_server_conf_t srv_conf = {
  NULL,
  NULL,
  run,
  0,
  4,
};


static bool
pf_init(void) {
  int port = spe_opt_int("pf_base", "port", 7879);
  int sfd = spe_sock_tcp_server("127.0.0.1", port);
  if (sfd < 0) {
    printf("server socket create error\n");
    return false;
  }
  srv = spe_server_create(sfd, &srv_conf);
  if (!srv) {
    printf("server create error\n");
    return false;
  }
  return true;
}

static bool
pf_start(void) {
  spe_server_start(srv);
  return true;
}

static void
pf_before_loop(void) {
  spe_server_before_loop(srv);
}

static void
pf_after_loop(void) {
  spe_server_after_loop(srv);
}

static spe_module_t pf_module = {
  pf_init,
  NULL,
  pf_start,
  NULL, 
  pf_before_loop,
  pf_after_loop,
};

__attribute__((constructor))
static void
__pf_init(void) {
  spe_register_module(&pf_module);
}
