#include "spe.h"
#include <stdio.h>

static void
on_conn(spe_conn_t* conn) {
  if (conn->ConnectTimeout) {
    printf("Connect Timeout\n");
  }
  if (conn->Error) {
    printf("Connect Error\n");
  }
  spe_conn_destroy(conn);
  g_stop = true;
}

bool
mod_init(void) {
  int cfd = spe_sock_tcp_socket();
  spe_conn_t *conn = spe_conn_create(cfd);
  conn->ReadCallback.Handler = SPE_HANDLER1(on_conn, conn);
  spe_conn_connect(conn, "127.0.0.1", "8080");
  return true;
}

bool
mod_exit(void) {
  return true;
}
