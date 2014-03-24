#include "spe_redis.h"
#include "spe_sock.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdio.h>
#include <stdarg.h>

static void
on_connect(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  SPE_HANDLER_CALL(sr->handler);
}

/*
===================================================================================================
spe_redis_connect
===================================================================================================
*/
void
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler) {
  ASSERT(sr);
  sr->handler = handler;
  spe_conn_connect(sr->conn, sr->host, sr->port, SPE_HANDLER1(on_connect, sr));
}

static void
on_receive(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  SPE_HANDLER_CALL(sr->handler);
}

static void
on_send(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  if (sr->error || sr->closed) {
    SPE_HANDLER_CALL(sr->handler);
    return;
  }
  spe_conn_readuntil(conn, "\r\n", SPE_HANDLER1(on_receive, sr));
}

/*
===================================================================================================
spe_redis_get
===================================================================================================
*/
void
spe_redis_get(spe_redis_t* sr, spe_string_t* key, spe_handler_t handler) {
  ASSERT(sr && key);
  sr->handler = handler;
  spe_conn_writes(sr->conn, "*2\r\n");
  spe_conn_writes(sr->conn, "$3\r\nGET\r\n");
  char buf[1024];
  sprintf(buf, "$%d\r\n%s\r\n", key->len, key->data);
  spe_conn_writes(sr->conn, buf);
  spe_conn_flush(sr->conn, SPE_HANDLER1(on_send, sr));
}

/*
===================================================================================================
spe_redis_set
===================================================================================================
*/
void
spe_redis_set(spe_redis_t* sr, spe_string_t* key, spe_string_t* value, spe_handler_t handler) {
  ASSERT(sr && key && value);
  sr->handler = handler;
  spe_conn_writes(sr->conn, "*3\r\n");
  spe_conn_writes(sr->conn, "$3\r\nSET\r\n");
  char buf[1024];
  sprintf(buf, "$%d\r\n%s\r\n$%d\r\n", key->len, key->data, value->len);
  spe_conn_writes(sr->conn, buf);
  spe_conn_write(sr->conn, value);
  spe_conn_writes(sr->conn, "\r\n");
  spe_conn_flush(sr->conn, SPE_HANDLER1(on_send, sr));
}

static void
spe_redis_send(spe_redis_t* sr) {
  char buf[1024];
  sprintf(buf, "*%d\r\n", sr->send_buffer->len);
  spe_conn_writes(sr->conn, buf);
  for (int i=0; i<sr->send_buffer->len; i++) {
    sprintf(buf, "$%d\r\n%s\r\n", sr->send_buffer->data[i]->len, sr->send_buffer->data[i]->data);
    spe_conn_writes(sr->conn, buf);
  }
  spe_conn_flush(sr->conn, SPE_HANDLER1(on_send, sr));
}

/*
===================================================================================================
spe_redis_do
===================================================================================================
*/
void
spe_redis_do(spe_redis_t* sr, spe_handler_t handler, int nargs, ...) {
  ASSERT(sr && nargs>0);
  sr->handler = handler;
  spe_slist_clean(sr->send_buffer);
  char* key;
  va_list ap;
  va_start(ap, nargs);
  for (int i=0; i<nargs; i++) {
    key = va_arg(ap, char*);
    spe_slist_appends(sr->send_buffer, key);
  }
  key = va_arg(ap, char*);
  va_end(ap);
  spe_redis_send(sr);
}

/*
===================================================================================================
spe_redis_create
===================================================================================================
*/
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
    goto failout1;
  }
  if (!(sr->conn = spe_conn_create(cfd))) {
    SPE_LOG_ERR("spe_conn_create error");
    goto failout2;
  }
  sr->host = host;
  sr->port = port;

  if (!(sr->send_buffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout2;
  }
  if (!(sr->recv_buffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout3;
  }
  return sr;

failout3:
  spe_slist_destroy(sr->send_buffer);
failout2:
  spe_sock_close(cfd);
failout1:
  free(sr);
  return NULL;
}
