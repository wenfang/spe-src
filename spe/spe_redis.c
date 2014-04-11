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
  if (sr->error || sr->closed) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
  }
  SPE_HANDLER_CALL(sr->handler);
}

/*
===================================================================================================
spe_redis_connect
===================================================================================================
*/
bool
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler) {
  ASSERT(sr && sr->host && sr->port);
  if (sr->conn) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
  }
  int cfd = spe_sock_tcp_socket();
  if (cfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_socket error");
    return false;
  }
  if (!(sr->conn = spe_conn_create(cfd))) {
    SPE_LOG_ERR("spe_conn_create error");
    spe_sock_close(cfd);
    return false;
  }
  sr->handler = handler;
  sr->closed  = 0;
  sr->error   = 0;
  spe_conn_connect(sr->conn, sr->host, sr->port, SPE_HANDLER1(on_connect, sr));
  return true;
}

static void
on_receive_data(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  if (sr->error || sr->closed) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
  }
  spe_slist_appendb(sr->recv_buffer, conn->buffer->data, conn->buffer->len-2);
  SPE_HANDLER_CALL(sr->handler);
}

static void
on_receive_line(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  if (sr->error || sr->closed) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
    SPE_HANDLER_CALL(sr->handler);
    return;
  }
  // one line response
  if (conn->buffer->data[0] == '+' || conn->buffer->data[0] == '-' ||
      conn->buffer->data[0] == ':') {
    spe_slist_append(sr->recv_buffer, conn->buffer);
    SPE_HANDLER_CALL(sr->handler);
    return;
  }
  // block data
  if (conn->buffer->data[0] == '$') {
    spe_slist_append(sr->recv_buffer, conn->buffer);
    int resSize = atoi(conn->buffer->data+1);
    if (resSize <= 0) {
      SPE_HANDLER_CALL(sr->handler);
      return;
    }
    spe_conn_readbytes(conn, resSize+2, SPE_HANDLER1(on_receive_data, sr));
  }
}

static void
on_send(void* arg) {
  spe_redis_t* sr = arg;
  spe_conn_t* conn = sr->conn;
  if (conn->error) sr->error = 1;
  if (conn->closed) sr->closed = 1;
  if (sr->error || sr->closed) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
    SPE_HANDLER_CALL(sr->handler);
    return;
  }
  spe_conn_readuntil(conn, "\r\n", SPE_HANDLER1(on_receive_line, sr));
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
bool
spe_redis_do(spe_redis_t* sr, spe_handler_t handler, int nargs, ...) {
  ASSERT(sr && nargs>0);
  if (!sr->conn) return false;
  // init redis for new command
  sr->handler = handler;
  spe_slist_clean(sr->send_buffer);
  spe_slist_clean(sr->recv_buffer);
  // generate send command
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
  return true;
}

/*
===================================================================================================
spe_redis_create
===================================================================================================
*/
spe_redis_t*
spe_redis_create(const char* host, const char* port) {
  // set host and port
  spe_redis_t* sr = calloc(1, sizeof(spe_redis_t));
  if (!sr) {
    SPE_LOG_ERR("spe_redis calloc error");
    return NULL;
  }
  sr->host = host;
  sr->port = port;
  // init buffer
  if (!(sr->send_buffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout1;
  }
  if (!(sr->recv_buffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout2;
  }
  return sr;

failout2:
  spe_slist_destroy(sr->send_buffer);
failout1:
  free(sr);
  return NULL;
}

/*
===================================================================================================
spe_redis_destroy
===================================================================================================
*/
void
spe_redis_destroy(spe_redis_t* sr) {
  ASSERT(sr);
  spe_slist_destroy(sr->send_buffer);
  spe_slist_destroy(sr->recv_buffer);
  if (sr->conn) spe_conn_destroy(sr->conn);
  free(sr);
}
