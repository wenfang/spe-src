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
  spe_conn_t* conn = pf_conn->conn;
  if (conn->Closed || conn->Error) {
    SpeConnDestroy(conn);
    free(pf_conn);
    return;
  }

  cJSON *obj;
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
      SpeConnFlush(conn);
      break;
    case PF_END:
      SpeConnDestroy(conn);
      free(pf_conn);
      break;
  }
}

static void
run(spe_conn_t* conn) {
  pf_conn_t* pf_conn = calloc(1, sizeof(pf_conn_t));
  if (!pf_conn) {
    SpeConnDestroy(conn);
    return;
  }
  conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, pf_conn);
  conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, pf_conn);
  pf_conn->conn   = conn;
  pf_conn->status = PF_INIT;
  spe_conn_readuntil(pf_conn->conn, "\r\n\r\n");
}

bool
mod_init(void) {
  /*
  int procs = SpeOptInt("pf_base", "procs", 4);
  SpeProcs(procs);
  SpeTpoolInit();
  */
  if (!SpeServerSetHandler(run)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  return true;
}

bool
mod_exit(void) {
  /*
  SpeTpoolDeinit();
  */
  return true;
}
