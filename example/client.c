#include "spe.h"
#include <stdio.h>

static void
on_conn(SpeConn_t* conn) {
  if (conn->ConnectTimeout) {
    printf("Connect Timeout\n");
  }
  if (conn->Error) {
    printf("Connect Error\n");
  }
  SpeConnDestroy(conn);
  GStop = true;
}

bool
mod_init(void) {
  int cfd = spe_sock_tcp_socket();
  SpeConn_t *conn = SpeConnCreate(cfd);
  conn->ReadCallback.Handler = SPE_HANDLER1(on_conn, conn);
  SpeConnConnect(conn, "127.0.0.1", "8080");
  return true;
}

bool
mod_exit(void) {
  return true;
}
