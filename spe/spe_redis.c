#include "spe_redis.h"
#include "spe_sock.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdio.h>
#include <stdarg.h>

#define SPE_REDIS_INIT      0
#define SPE_REDIS_CONN      1
#define SPE_REDIS_SEND      2
#define SPE_REDIS_RECV_LINE 3
#define SPE_REDIS_RECV_DATA 4

static void 
driver_machine(spe_redis_t* sr) {
  if (sr->status != SPE_REDIS_INIT && (sr->conn->error || sr->conn->closed)) {
    spe_conn_destroy(sr->conn);
    sr->conn = NULL;
    sr->status = SPE_REDIS_INIT;
    sr->error = 1;
    SPE_HANDLER_CALL(sr->handler);
    return;
  }
  int cfd;
  char buf[1024];
  switch (sr->status) {
    case SPE_REDIS_INIT:
      cfd = spe_sock_tcp_socket();
      if (cfd < 0) {
        SPE_LOG_ERR("spe_sock_tcp_socket error");
        sr->error = 1;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      if (!(sr->conn = spe_conn_create(cfd))) {
        SPE_LOG_ERR("spe_conn_create error");
        spe_sock_close(cfd);
        sr->error = 1;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      sr->error = 0;
      sr->status = SPE_REDIS_CONN;
      sr->conn->read_callback_task.handler = SPE_HANDLER1(driver_machine, sr);
      sr->conn->write_callback_task.handler = SPE_HANDLER1(driver_machine, sr);
      spe_conn_connect(sr->conn, sr->host, sr->port);
      break;
    case SPE_REDIS_CONN:
      // send request
      sprintf(buf, "*%d\r\n", sr->send_buffer->len);
      spe_conn_writes(sr->conn, buf);
      for (int i=0; i<sr->send_buffer->len; i++) {
        sprintf(buf, "$%d\r\n%s\r\n", sr->send_buffer->data[i]->len, sr->send_buffer->data[i]->data);
        spe_conn_writes(sr->conn, buf);
      }
      sr->status = SPE_REDIS_SEND;
      spe_conn_flush(sr->conn);
    case SPE_REDIS_SEND:
      sr->status = SPE_REDIS_RECV_LINE;
      spe_conn_readuntil(sr->conn, "\r\n");
      break;
    case SPE_REDIS_RECV_LINE:
      if (sr->conn->buffer->data[0] == '+' || sr->conn->buffer->data[0] == '-' ||
          sr->conn->buffer->data[0] == ':') {
        spe_slist_append(sr->recv_buffer, sr->conn->buffer);
        sr->status = SPE_REDIS_CONN;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      if (sr->conn->buffer->data[0] == '$') {
        spe_slist_append(sr->recv_buffer, sr->conn->buffer);
        int resSize = atoi(sr->conn->buffer->data+1);
        if (resSize <= 0) {
          sr->status = SPE_REDIS_CONN;
          SPE_HANDLER_CALL(sr->handler);
          return;
        }
        sr->status = SPE_REDIS_RECV_DATA;
        spe_conn_readbytes(sr->conn, resSize+2);
        return;
      }
      if (sr->conn->buffer->data[0] == '*') {
        // TODO: ADD BLOCK SUPPOR
        return;
      }
      break;
    case SPE_REDIS_RECV_DATA:
      spe_slist_appendb(sr->recv_buffer, sr->conn->buffer->data, sr->conn->buffer->len-2);
      sr->status = SPE_REDIS_CONN;
      SPE_HANDLER_CALL(sr->handler);
      break;
  }
}

/*
===================================================================================================
spe_redis_do
===================================================================================================
*/
void
spe_redis_do(spe_redis_t* sr, spe_handler_t handler, int nargs, ...) {
  ASSERT(sr && nargs>0);
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
  driver_machine(sr);
}

/*
===================================================================================================
spe_redis_create
===================================================================================================
*/
spe_redis_t*
spe_redis_create(const char* host, const char* port) {
  ASSERT(host && port);
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
  sr->status = SPE_REDIS_INIT;
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
