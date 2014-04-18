#ifndef __SPE_MYSQL_H
#define __SPE_MYSQL_H

struct spe_mysql_s {
  spe_conn_t* conn;
  const char* host;
  const char* port;
  const char* user;
  const char* passwd;
  const char* dbname;
};
typedef struct spe_mysql_s spe_mysql_t;

extern spe_mysql_t*
spe_mysql_create(const char* host, const char* port, const char* user, 
    const char* passwd, const char* dbname);

#endif
