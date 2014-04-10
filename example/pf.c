#include "spe.h"
#include <stdio.h>


static spe_server_t* srv;

void
on_close(void* arg1) {
  spe_conn_t* conn = arg1;
  spe_conn_destroy(conn);
}

void
on_get(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  for(int i=0; i<red->recv_buffer->len; i++) {
    spe_conn_write(conn, red->recv_buffer->data[i]);
  }
  spe_redis_destroy(red);
  spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
}

void
on_conn(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  spe_redis_do(red, SPE_HANDLER2(on_get, conn, red), 2, "get", "mydokey");
}

void
on_read(void* arg) {
  spe_conn_t* conn = arg;
  if (conn->closed || conn->error) {
    spe_conn_destroy(conn);
    return;
  }
  spe_redis_t* red = spe_redis_create("127.0.0.1", "6379");
  spe_redis_connect(red, SPE_HANDLER2(on_conn, conn, red));
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
  4,
};


static bool
pf_base_init(void) {
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
pf_base_start(void) {
  spe_server_enable(srv);
  return true;
}

static spe_module_t pf_base_module = {
  pf_base_init,
  NULL,
  pf_base_start,
  NULL,
};

__attribute__((constructor))
static void
__pf_base_init(void) {
  spe_register_module(&pf_base_module);
}
