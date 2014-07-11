#include "spe.h"
#include <stdio.h>

#define PF_INIT 0
#define PF_END  1

struct pf_conn_s {
  spe_conn_t* conn;
  int         status;
};
typedef struct pf_conn_s pf_conn_t;

static void
driver_machine(void* arg) {
  pf_conn_t* pf_conn = arg;
  cJSON *obj;
  spe_conn_t* conn = pf_conn->conn;
  if (conn->closed || conn->error) {
    spe_conn_destroy(conn);
    free(pf_conn);
    return;
  }
  switch (pf_conn->status) {
    case PF_INIT:
      obj = cJSON_CreateObject();
      cJSON_AddNumberToObject(obj, "res", 0);
      cJSON_AddStringToObject(obj, "msg", "OK");
      pf_conn->status = PF_END;
      char* msg = cJSON_PrintUnformatted(obj);
      spe_conn_writes(conn, msg);
      free(msg);
      cJSON_Delete(obj);
      spe_conn_flush(conn);
      break;
    case PF_END:
      spe_conn_destroy(conn);
      free(pf_conn);
      break;
  }
}

static void
run(unsigned cfd) {
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

bool
mod_init(void) {
  int port = SpeOptInt("pf_base", "port", 7879);
  int procs = SpeOptInt("pf_base", "procs", 4);
  SpeServerInit("127.0.0.1", port, run);
  spe_main_procs(procs);
  return true;
}

bool
mod_exit(void) {
  SpeServerDeinit();
  return true;
}
