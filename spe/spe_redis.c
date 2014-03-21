#include "spe_redis.h"
#include "spe_sock.h"
#include "spe_log.h"
#include "spe_util.h"

void
spe_redis_connect(spe_redis_t* sr, spe_handler_t handler) {
  ASSERT(sr);
  spe_conn_connect(sr->conn, sr->host, sr->port, handler);
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
