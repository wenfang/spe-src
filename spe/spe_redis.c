#include "spe_redis.h"
#include "spe_sock.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdio.h>

void
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler) {
  ASSERT(sr);
  spe_conn_connect(sr->conn, sr->host, sr->port, handler);
}

static void
on_send_ack(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  fprintf(stderr, "receive: %s\n", conn->buffer->data);
  SPE_HANDLER_CALL(sr->write_handler);
}

static void
on_send(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  if (sr->error || sr->closed) {
    SPE_HANDLER_CALL(sr->write_handler);
    return;
  }
  fprintf(stderr, "on_send ok\n");
  spe_conn_readuntil(conn, "\r\n", SPE_HANDLER1(on_send_ack, sr));
}

void
spe_redis_set(spe_redis_t* sr, spe_string_t* key, spe_string_t* value, spe_handler_t handler) {
  ASSERT(sr && key && value);
  sr->write_handler = handler;
  char buf[1024];
  spe_conn_writes(sr->conn, "*3\r\n");
  spe_conn_writes(sr->conn, "$3\r\nSET\r\n");
  sprintf(buf, "$%d\r\n%s\r\n$%d\r\n", key->len, key->data, value->len);
  spe_conn_writes(sr->conn, buf);
  spe_conn_write(sr->conn, value);
  spe_conn_writes(sr->conn, "\r\n");
  spe_conn_flush(sr->conn, SPE_HANDLER1(on_send, sr));
}

spe_redis_t*
spe_redis_create(const char* host, const char* port) {
  spe_redis_t* sr = calloc(1, sizeof(spe_redis_t));
  if (!sr) {
    SPE_LOG_ERR("spe_redis calloc error");
    return NULL;
  }
  int cfd = spe_sock_tcp_socket();
  if (cfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_socket error");
    free(sr);
    return NULL;
  }
  sr->conn = spe_conn_create(cfd);
  if (!sr->conn) {
    SPE_LOG_ERR("spe_conn_create error");
    spe_sock_close(cfd);
    free(sr);
    return NULL;
  }
  sr->host = host;
  sr->port = port;
  return sr;
}
