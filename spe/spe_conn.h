#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_string.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

struct spe_conn_s {
  int             _fd;
  SpeTask_t       _read_task;
  SpeTask_t       _write_task;
  unsigned        _read_expire_time;
  unsigned        _write_expire_time;
  SpeTask_t       read_callback_task;
  SpeTask_t       write_callback_task;
  spe_string_t*   _read_buffer;
  spe_string_t*   _write_buffer;
  spe_string_t*   buffer;
  char*           _delim;
  unsigned        _rbytes;
  unsigned        _init;
  unsigned        _read_type:2;
  unsigned        _write_type:1;
  unsigned        connect_timeout:1;
  unsigned        read_timeout:1;
  unsigned        write_timeout:1;
  unsigned        closed:1;
  unsigned        error:1;
};
typedef struct spe_conn_s spe_conn_t;

extern bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port);

extern bool
spe_conn_readuntil(spe_conn_t* conn, char* delim);

extern bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len);

extern bool
spe_conn_read(spe_conn_t* conn);

static inline bool
spe_conn_writeb(spe_conn_t* conn, char* buf, unsigned len) {
  ASSERT(conn && buf);
  if (conn->closed || conn->error) return false;
  return spe_string_catb(conn->_write_buffer, buf, len);
}

static inline bool
spe_conn_write(spe_conn_t* conn, spe_string_t* buf) {
  ASSERT(conn && buf);
  if (conn->closed || conn->error) return false;
  return spe_string_catb(conn->_write_buffer, buf->data, buf->len);
}

static inline bool
spe_conn_writes(spe_conn_t* conn, char* buf) {
  ASSERT(conn && buf);
  if (conn->closed || conn->error) return false;
  return spe_string_catb(conn->_write_buffer, buf, strlen(buf));
}

extern bool
spe_conn_flush(spe_conn_t* conn);

extern bool
spe_conn_set_timeout(spe_conn_t* conn, unsigned read_expire_time, unsigned write_expire_time);

extern spe_conn_t*
spe_conn_create(unsigned fd);

extern void
spe_conn_destroy(spe_conn_t* conn);

#endif
