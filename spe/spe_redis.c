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

/*
===================================================================================================
speRedisCreate
===================================================================================================
*/
static SpeRedis_t*
speRedisCreate(const char* host, const char* port) {
  ASSERT(host && port);
  SpeRedis_t* sr = calloc(1, sizeof(SpeRedis_t));
  if (!sr) {
    SPE_LOG_ERR("spe_redis calloc error");
    return NULL;
  }
  sr->host = host;
  sr->port = port;
  // init buffer
  if (!(sr->sendBuffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout1;
  }
  if (!(sr->recvBuffer = spe_slist_create())) {
    SPE_LOG_ERR("spe_slist_create error");
    goto failout2;
  }
  sr->status = SPE_REDIS_INIT;
  return sr;

failout2:
  spe_slist_destroy(sr->sendBuffer);
failout1:
  free(sr);
  return NULL;
}

/*
===================================================================================================
speRedisDestroy
===================================================================================================
*/
static void
speRedisDestroy(SpeRedis_t* sr) {
  ASSERT(sr);
  spe_slist_destroy(sr->sendBuffer);
  spe_slist_destroy(sr->recvBuffer);
  if (sr->conn) spe_conn_destroy(sr->conn);
  free(sr);
}

/*
===================================================================================================
speRedisGet
===================================================================================================
*/
SpeRedis_t*
SpeRedisPoolGet(SpeRedisPool_t* srp) {
  ASSERT(srp);
  if (!srp->len) return speRedisCreate(srp->host, srp->port);
  return srp->poolData[--srp->len];
}

/*
===================================================================================================
speRedisPut
===================================================================================================
*/
void
SpeRedisPoolPut(SpeRedisPool_t* srp, SpeRedis_t* sr) {
  ASSERT(srp && sr);
  if (srp->len == srp->size) {
    speRedisDestroy(sr);
    return; 
  }
  srp->poolData[srp->len++] = sr;
}

/*
===================================================================================================
driver_machine
===================================================================================================
*/
static void 
driver_machine(SpeRedis_t* sr) {
  if (sr->status != SPE_REDIS_INIT && (sr->conn->Error || sr->conn->Closed)) {
    spe_conn_destroy(sr->conn);
    sr->conn    = NULL;
    sr->status  = SPE_REDIS_INIT;
    sr->Error   = 1;
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
        sr->Error = 1;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      if (!(sr->conn = spe_conn_create(cfd))) {
        SPE_LOG_ERR("spe_conn_create error");
        spe_sock_close(cfd);
        sr->Error = 1;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      sr->Error   = 0;
      sr->status  = SPE_REDIS_CONN;
      sr->conn->ReadCallback.Handler  = SPE_HANDLER1(driver_machine, sr);
      sr->conn->WriteCallback.Handler = SPE_HANDLER1(driver_machine, sr);
      spe_conn_connect(sr->conn, sr->host, sr->port);
      break;
    case SPE_REDIS_CONN:
      // send request
      sprintf(buf, "*%d\r\n", sr->sendBuffer->len);
      spe_conn_writes(sr->conn, buf);
      for (int i=0; i<sr->sendBuffer->len; i++) {
        sprintf(buf, "$%d\r\n%s\r\n", sr->sendBuffer->data[i]->len, sr->sendBuffer->data[i]->data);
        spe_conn_writes(sr->conn, buf);
      }
      sr->status = SPE_REDIS_SEND;
      spe_conn_flush(sr->conn);
      break;
    case SPE_REDIS_SEND:
      sr->status = SPE_REDIS_RECV_LINE;
      spe_conn_readuntil(sr->conn, "\r\n");
      break;
    case SPE_REDIS_RECV_LINE:
      if (sr->conn->Buffer->data[0] == '+' || sr->conn->Buffer->data[0] == '-' ||
          sr->conn->Buffer->data[0] == ':') {
        spe_slist_append(sr->recvBuffer, sr->conn->Buffer);
        sr->status = SPE_REDIS_CONN;
        SPE_HANDLER_CALL(sr->handler);
        return;
      }
      if (sr->conn->Buffer->data[0] == '$') {
        spe_slist_append(sr->recvBuffer, sr->conn->Buffer);
        int resSize = atoi(sr->conn->Buffer->data+1);
        if (resSize <= 0) {
          sr->status = SPE_REDIS_CONN;
          SPE_HANDLER_CALL(sr->handler);
          return;
        }
        sr->status = SPE_REDIS_RECV_DATA;
        spe_conn_readbytes(sr->conn, resSize+2);
        return;
      }
      if (sr->conn->Buffer->data[0] == '*') {
        // TODO: ADD BLOCK SUPPOR
        return;
      }
      break;
    case SPE_REDIS_RECV_DATA:
      spe_slist_appendb(sr->recvBuffer, sr->conn->Buffer->data, sr->conn->Buffer->len-2);
      sr->status = SPE_REDIS_CONN;
      SPE_HANDLER_CALL(sr->handler);
      break;
  }
}

/*
===================================================================================================
speRedisPoolDo
===================================================================================================
*/
bool
SpeRedisDo(SpeRedis_t* sr, spe_handler_t handler, int nargs, ...) {
  ASSERT(sr && nargs>0);
  // init redis for new command
  sr->handler = handler;
  spe_slist_clean(sr->sendBuffer);
  spe_slist_clean(sr->recvBuffer);
  // generate send command
  char* key;
  va_list ap;
  va_start(ap, nargs);
  for (int i=0; i<nargs; i++) {
    key = va_arg(ap, char*);
    spe_slist_appends(sr->sendBuffer, key);
  }
  key = va_arg(ap, char*);
  va_end(ap);
  driver_machine(sr);
  return true;
}

/*
===================================================================================================
SpeRedisPoolCreate
===================================================================================================
*/
SpeRedisPool_t*
SpeRedisPoolCreate(const char* host, const char* port, unsigned size) {
  ASSERT(host && port);
  SpeRedisPool_t* srp = calloc(1, sizeof(SpeRedisPool_t) + size * sizeof(SpeRedis_t*));
  if (!srp) {
    SPE_LOG_ERR("SpeRedisPool calloc Error");
    return NULL;
  }
  srp->host = host;
  srp->port = port;
  srp->size = size;
  srp->len  = 0;
  return srp;
}

/*
===================================================================================================
SpeRedisPoolDestroy
===================================================================================================
*/
void
SpeRedisPoolDestroy(SpeRedisPool_t* srp) {
  ASSERT(srp);
  for (int i=0; i<srp->len; i++) speRedisDestroy(srp->poolData[i]);
  free(srp);
}
