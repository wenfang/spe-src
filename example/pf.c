#include "spe.h"
#include <stdio.h>

#define PF_INIT 0
#define PF_END  1

struct pf_conn_s {
  SpeConn_t* conn;
  int         status;
};
typedef struct pf_conn_s pf_conn_t;

static void
driver_machine(void* arg) {
  pf_conn_t* pf_conn = arg;
  SpeConn_t* conn = pf_conn->conn;
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
      cJSON_AddStringToObject(obj, "comment", "connect message");
      pf_conn->status = PF_END;
      char* msg = cJSON_PrintUnformatted(obj);
      SpeConnWrites(conn, msg);
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
run(SpeConn_t* conn) {
  pf_conn_t* pf_conn = calloc(1, sizeof(pf_conn_t));
  if (!pf_conn) {
    SpeConnDestroy(conn);
    return;
  }
  conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, pf_conn);
  conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, pf_conn);
  pf_conn->conn   = conn;
  pf_conn->status = PF_INIT;
  SpeConnReaduntil(pf_conn->conn, "\r\n\r\n");
}

bool
mod_init(void) {
  if (!SpeServerInit("127.0.0.1", 7879, run)) {
    fprintf(stderr, "SpeServerSetHandler Error\n");
    return false;
  }
  return true;
}

bool
mod_exit(void) {
  SpeServerDeinit();
  return true;
}
