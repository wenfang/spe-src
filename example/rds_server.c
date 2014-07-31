#include "spe.h"
#include <stdbool.h>

static SpeRedisPool_t* pool;

#define RDS_INIT  0
#define RDS_GET   1
#define RDS_CLOSE 2

struct spe_rds_s {
  spe_conn_t* conn;
  SpeRedis_t* red;
  unsigned    status;
};
typedef struct spe_rds_s spe_rds_t;

static void
drive_machine(void* arg) {
  spe_rds_t* rds = arg;
  cJSON *obj;
  if (rds->red && rds->red->Error && rds->status != RDS_CLOSE) {
    rds->status = RDS_CLOSE;
    spe_conn_writes(rds->conn, "ERROR GET DATA\r\n");
    SpeConnFlush(rds->conn);
    return;
  }
  switch (rds->status) {
    case RDS_INIT:
      if (rds->conn->Error || rds->conn->Closed) {
        SpeConnDestroy(rds->conn);
        free(rds);
        return;
      }
      rds->red = SpeRedisPoolGet(pool);
      if (!rds->red) {
        rds->status = RDS_CLOSE;
        spe_conn_writes(rds->conn, "ERROR CREATE REDIS CLIENT\r\n");
        SpeConnFlush(rds->conn);
        return;
      }
      rds->status = RDS_GET;
      SpeRedisGet(rds->red, SPE_HANDLER1(drive_machine, rds), "mydokey");
      break;
    case RDS_GET:
      obj = cJSON_CreateObject();
      cJSON_AddNumberToObject(obj, "res", 0);
      cJSON_AddStringToObject(obj, "msg", rds->red->Buffer->data[0]->data);
      char* msg = cJSON_PrintUnformatted(obj);
      spe_conn_writes(rds->conn, msg);
      free(msg);
      cJSON_Delete(obj);
      rds->status = RDS_CLOSE;
      SpeConnFlush(rds->conn);
      break;
    case RDS_CLOSE:
      if (rds->red) SpeRedisPoolPut(pool, rds->red);
      SpeConnDestroy(rds->conn);
      free(rds);
      break;
  }
}

static void
run(spe_conn_t* conn) {
  spe_rds_t* rds = calloc(1, sizeof(spe_rds_t));
  if (!rds) {
    SPE_LOG_ERR("calloc error");
    SpeConnDestroy(conn);
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
  if (!SpeServerSetHandler(run)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  pool = SpeRedisPoolCreate("127.0.0.1", "6379", 128);
  if (!pool) {
    fprintf(stderr, "redis pool create error\n");
    return false;
  }
  return true;
}

bool
mod_exit(void) {
  SpeRedisPoolDestroy(pool);
  return true;
}
