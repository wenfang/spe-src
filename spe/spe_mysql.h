#ifndef __SPE_MYSQL_H
#define __SPE_MYSQL_H

struct spe_mysql_s {
  spe_conn_t* conn;
  const char* host;
  const char* port;
};
typedef struct spe_mysql_s spe_mysql_t;

#endif
