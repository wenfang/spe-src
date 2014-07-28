#ifndef __SPE_MYSQL_H
#define __SPE_MYSQL_H

struct SpeMysql_s {
  spe_conn_t* conn;
  const char* host;
  const char* port;
};
typedef struct SpeMysql_s SpeMysql_t;

struct SpeMysqlPool_s {
  const char* host;
  const char* port;
  unsigned    size;
  unsigned    len;
  SpeMysql_t* poolData[0];
};
typedef struct SpeMysqlPool_s SpeMysqlPool_t;

#endif
