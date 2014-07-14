#include "spe.h"
#include <stdbool.h>

static spe_pool_t* pool;

#define RDS_INIT  0
#define RDS_GET   1
#define RDS_CLOSE 2

struct spe_rds_s {
  spe_conn_t*   conn;
  spe_redis_t*  red;
  unsigned      status;
};
typedef struct spe_rds_s spe_rds_t;

static void
drive_machine(void* arg) {
  spe_rds_t* rds = arg;
  if (rds->red && rds->red->error && rds->status != RDS_CLOSE) {
    rds->status = RDS_CLOSE;
    spe_conn_writes(rds->conn, "ERROR GET DATA\r\n");
    spe_conn_flush(rds->conn);
    spe_redis_destroy(rds->red);
    rds->red = NULL;
    return;
  }
  switch (rds->status) {
    case RDS_INIT:
      if (rds->conn->Error || rds->conn->closed) {
        spe_conn_destroy(rds->conn);
        free(rds);
        return;
      }
      rds->red = spe_pool_get(pool);
      if (!rds->red) {
        rds->red = spe_redis_create("127.0.0.1", "6379");
        if (!rds->red) {
          rds->status = RDS_CLOSE;
          spe_conn_writes(rds->conn, "ERROR CREATE REDIS CLIENT\r\n");
          spe_conn_flush(rds->conn);
          return;
        }
      }
      rds->status = RDS_GET;
      spe_redis_do(rds->red, SPE_HANDLER1(drive_machine, rds), 2, "get", "mydokey");
      break;
    case RDS_GET:
      for (int i=0; i<rds->red->recv_buffer->len; i++) {
        spe_conn_write(rds->conn, rds->red->recv_buffer->data[i]);
      }
      rds->status = RDS_CLOSE;
      spe_conn_flush(rds->conn);
      break;
    case RDS_CLOSE:
      if (rds->red) spe_pool_put(pool, rds->red);
      spe_conn_destroy(rds->conn);
      free(rds);
      break;
  }
}

static void
run(unsigned cfd) {
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
  rds->conn->ReadCallback.Handler   = SPE_HANDLER1(drive_machine, rds);
  rds->conn->WriteCallback.Handler  = SPE_HANDLER1(drive_machine, rds);
  spe_conn_readuntil(rds->conn, "\r\n\r\n");
}

bool
mod_init(void) {
  int port = SpeOptInt("rdspool", "port", 7879);
  pool = spe_pool_create(128, (spe_pool_Free)spe_redis_destroy);
  if (!pool) {
    fprintf(stderr, "redis pool create error\n");
    return false;
  }
  SpeServerInit("127.0.0.1", port, run);
  return true;
}

bool
mod_exit(void) {
  SpeServerDeinit();
  return true;
}
