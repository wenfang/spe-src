#include "spe.h"
#include <stdbool.h>

static spe_server_t* srv;

static void
on_close(void* arg) {
  spe_conn_t* conn = arg;
  spe_conn_destroy(conn);
}

static void
on_get(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  if (red->closed || red->error) {
    spe_conn_writes(conn, "ERROR on_get\r\n");
    spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
    return;
  }
  for (int i=0; i<red->recv_buffer->len; i++) {
    spe_conn_write(conn, red->recv_buffer->data[i]);
  }
  spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
  spe_redis_destroy(red);
}

static void
on_redis_connect(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  if (red->closed || red->error) {
    spe_conn_writes(conn, "ERROR CONNECT REDIS SERVER\r\n");
    spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
    return;
  }
  spe_redis_do(red, SPE_HANDLER2(on_get, conn, red), 2, "get", "mydokey");
}

static void
on_read(void* arg) {
  spe_conn_t* conn = arg;
  spe_redis_t* red = spe_redis_create("127.0.0.1", "6379");
  if (!red) {
    spe_conn_writes(conn, "ERROR CREATE REDIS CLIENT\r\n");
    spe_conn_flush(conn, SPE_HANDLER1(on_close, conn));
    return;
  }
  spe_redis_connect(red, SPE_HANDLER2(on_redis_connect, conn, red));
}

static void
run(spe_server_t* srv, unsigned cfd) {
  spe_conn_t* conn = spe_conn_create(cfd);
  if (!conn) {
    spe_sock_close(cfd);
    return;
  }
  spe_conn_readuntil(conn, "\r\n\r\n", SPE_HANDLER1(on_read, conn));
}

static spe_server_conf_t srv_conf = {
  NULL,
  NULL,
  run,
  0,
  0,
};

static bool
rds_init(void) {
  int port = spe_opt_int("rds", "port", 7879);
  int sfd = spe_sock_tcp_server("127.0.0.1", port);
  if (sfd < 0) {
    fprintf(stderr, "server socket create error\n");
    return false;
  }
  srv = spe_server_create(sfd, &srv_conf);
  if (!srv) {
    fprintf(stderr, "server create error\n");
    return false;
  }
  return true;
}

static bool
rds_start(void) {
  spe_server_start(srv);
  return true;
}

static spe_module_t rds_module = {
  rds_init,
  NULL,
  rds_start,
};

__attribute__((constructor))
static void
__rds_init(void) {
  spe_register_module(&rds_module);
}
