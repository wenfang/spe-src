#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_string.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

struct spe_conn_s {
  SpeTask_t     ReadCallback;
  SpeTask_t     WriteCallback;
  int           fd;
  SpeTask_t     readTask;
  SpeTask_t     writeTask;
  unsigned      readExpireTime;
  unsigned      writeExpireTime;
  spe_string_t* readBuffer;
  spe_string_t* writeBuffer;
  spe_string_t* Buffer;
  char*         delim;
  unsigned      rbytes;
  unsigned      readType:2;
  unsigned      writeType:1;
  unsigned      ConnectTimeout:1;
  unsigned      ReadTimeout:1;
  unsigned      WriteTimeout:1;
  unsigned      Closed:1;
  unsigned      Error:1;
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
  ASSERT(conn && buf && len);
  if (conn->Closed || conn->Error) return false;
  return spe_string_catb(conn->writeBuffer, buf, len);
}

static inline bool
spe_conn_write(spe_conn_t* conn, spe_string_t* buf) {
  ASSERT(conn && buf);
  if (conn->Closed || conn->Error) return false;
  return spe_string_catb(conn->writeBuffer, buf->data, buf->len);
}

static inline bool
spe_conn_writes(spe_conn_t* conn, char* buf) {
  ASSERT(conn && buf);
  if (conn->Closed || conn->Error) return false;
  return spe_string_catb(conn->writeBuffer, buf, strlen(buf));
}

extern bool
SpeConnFlush(spe_conn_t* conn);

extern bool
spe_conn_set_timeout(spe_conn_t* conn, unsigned readExpireTime, unsigned writeExpireTime);

extern spe_conn_t*
SpeConnCreate(unsigned fd);

extern void
SpeConnDestroy(spe_conn_t* conn);

#endif
