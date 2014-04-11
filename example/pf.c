#include "spe.h"
#include <stdio.h>


static spe_server_t* srv;

static int count = 0;
void
on_close(void* arg1) {
  spe_conn_t* conn = arg1;
  spe_conn_destroy(conn);
  if (count++ > 100000) spe_main_stop = 1;
}

void
on_end(void* arg) {
  spe_conn_t* conn = arg;
  if (conn->closed || conn->error) {
    spe_conn_destroy(conn);
    return;
  }
  spe_conn_writes(conn, "OK\r\n");
  spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
}

static void
run(spe_server_t* srv, unsigned cfd) {
  spe_conn_t* conn = spe_conn_create(cfd);
  if (!conn) {
    spe_sock_close(cfd);
    return;
  }
  spe_conn_readuntil(conn, "\r\n\r\n", SPE_HANDLER1(on_end, conn));
}

static spe_server_conf_t srv_conf = {
  NULL,
  NULL,
  run,
  0,
  0,
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
