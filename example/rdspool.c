#include "spe.h"
#include <stdbool.h>

static spe_server_t* srv;
static spe_pool_t* pool;

#define RDS_INIT  0
#define RDS_GET   1
#define RDS_CLOSE 2
#define RDS_CONN  3

struct spe_rds_s {
  spe_conn_t*   conn;
  spe_redis_t*  red;
  unsigned      status;
};
typedef struct spe_rds_s spe_rds_t;

static void
on_close(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  if (red) {
    ASSERT(red->conn);
    ASSERT(red->conn->read_type == 0 && red->conn->write_type == 0);
    spe_pool_put(pool, red);
  }
  spe_conn_destroy(conn);
}

static void
on_get(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  if (red->closed || red->error) {
    spe_conn_writes(conn, "ERROR on_get\r\n");
    spe_conn_flush(conn, SPE_HANDLER2(on_close, conn, NULL));
    spe_redis_destroy(red);
    return;
  }
  for (int i=0; i<red->recv_buffer->len; i++) {
    spe_conn_write(conn, red->recv_buffer->data[i]);
  }
  spe_conn_flush(conn, SPE_HANDLER2(on_close, conn, red));
}

static void
on_redis_connect(void* arg1, void* arg2) {
  spe_conn_t* conn = arg1;
  spe_redis_t* red = arg2;
  if (red->closed || red->error) {
    spe_conn_writes(conn, "ERROR CONNECT REDIS SERVER\r\n");
    spe_conn_flush(conn, SPE_HANDLER2(on_close, conn, NULL));
    return;
  }
  ASSERT(red->conn);
  ASSERT(red->conn->read_type == 0 && red->conn->write_type == 0);
  bool res = spe_redis_do(red, SPE_HANDLER2(on_get, conn, red), 2, "get", "mydokey");
  ASSERT(res);
}

static void
on_read(void* arg) {
  spe_conn_t* conn = arg;
  spe_redis_t* red = spe_pool_get(pool);
  bool res;
  if (red) {
    ASSERT(red->conn);
    ASSERT(red->conn->read_type == 0 && red->conn->write_type == 0);
    res = spe_redis_do(red, SPE_HANDLER2(on_get, conn, red), 2, "get", "mydokey");
    ASSERT(res);
    return;
  }
  red = spe_redis_create("127.0.0.1", "6379");
  if (!red) {
    spe_conn_writes(conn, "ERROR CREATE REDIS CLIENT\r\n");
    spe_conn_flush(conn, SPE_HANDLER2(on_close, conn, NULL));
    return;
  }
  spe_redis_connect(red, SPE_HANDLER2(on_redis_connect, conn, red));
}

static void
drive_machine(void* arg) {
  spe_rds_t* rds = arg;
  if (rds->red && (rds->red->closed || rds->red->error) && rds->status != RDS_CLOSE) {
    rds->status = RDS_CLOSE;
    spe_conn_writes(rds->conn, "ERROR GET DATA\r\n");
    spe_conn_flush(rds->conn, SPE_HANDLER1(drive_machine, rds));
    spe_redis_destroy(rds->red);
    rds->red = NULL;
    return;
  }
  switch (rds->status) {
    case RDS_INIT:
      if (rds->conn->error || rds->conn->closed) {
        spe_conn_destroy(rds->conn);
        free(rds);
        return;
      }
      rds->red = spe_pool_get(pool);
      if (rds->red) {
        rds->status = RDS_GET;
        spe_redis_do(rds->red, SPE_HANDLER1(drive_machine, rds), 2, "get", "mydokey");
        return;
      }
      rds->red = spe_redis_create("127.0.0.1", "6379");
      if (!rds->red) {
        rds->status = RDS_CLOSE;
        spe_conn_writes(rds->conn, "ERROR CREATE REDIS CLIENT\r\n");
        spe_conn_flush(rds->conn, SPE_HANDLER1(drive_machine, rds));
        return;
      }
      rds->status = RDS_CONN;
      spe_redis_connect(rds->red, SPE_HANDLER1(drive_machine, rds));
      break;
    case RDS_GET:
      for (int i=0; i<rds->red->recv_buffer->len; i++) {
        spe_conn_write(rds->conn, rds->red->recv_buffer->data[i]);
      }
      rds->status = RDS_CLOSE;
      spe_conn_flush(rds->conn, SPE_HANDLER1(drive_machine, rds));
      break;
    case RDS_CLOSE:
      if (rds->red) spe_pool_put(pool, rds->red);
      spe_conn_destroy(rds->conn);
      free(rds);
      break;
    case RDS_CONN:
      rds->status = RDS_GET;
      spe_redis_do(rds->red, SPE_HANDLER1(drive_machine, rds), 2, "get", "mydokey");
      break;
  }
}

static void
run1(spe_server_t* srv, unsigned cfd) {
  spe_conn_t* conn = spe_conn_create(cfd);
  if (!conn) {
    SPE_LOG_ERR("spe_conn_create error");
    spe_sock_close(cfd);
    return;
  }
  spe_rds_t* rds = calloc(1, sizeof(spe_rds_t));
  if (!rds) {
    SPE_LOG_ERR("calloc error");
    spe_sock_close(cfd);
    return;
  }
  rds->conn = conn;
  rds->status = RDS_INIT;
  spe_conn_readuntil(conn, "\r\n\r\n", SPE_HANDLER1(drive_machine, rds));
}

void
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
  run1,
  0,
  0,
};

static bool
rdspool_init(void) {
  int port = spe_opt_int("rdspool", "port", 7879);
  pool = spe_pool_create(128, (spe_pool_Free)spe_redis_destroy);
  if (!pool) {
    fprintf(stderr, "redis pool create error\n");
    return false;
  }

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
rdspool_start(void) {
  spe_server_start(srv);
  return true;
}

static spe_module_t rdspool_module = {
  rdspool_init,
  NULL,
  rdspool_start,
};

__attribute__((constructor))
static void
__rdspool_init(void) {
  spe_register_module(&rdspool_module);
}
