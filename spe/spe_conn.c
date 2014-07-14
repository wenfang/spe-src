#include "spe_conn.h"
#include "spe_task.h"
#include "spe_sock.h"
#include "spe_util.h"
#include <errno.h>
#include <netdb.h>

#define BUF_SIZE  1024

#define SPE_CONN_READNONE   0
#define SPE_CONN_READ       1
#define SPE_CONN_READUNTIL  2
#define SPE_CONN_READBYTES  3

#define SPE_CONN_WRITENONE  0
#define SPE_CONN_WRITE      1

#define SPE_CONN_CONNECT    1

static spe_conn_t all_conn[MAX_FD];

static void
connect_generic(void* arg) {
  spe_conn_t* conn = arg;
  // connect timeout
  if (conn->readExpireTime && conn->readTask.Timeout) {
    conn->ConnectTimeout = 1;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->Error = 1;
  if (err) conn->Error = 1;
end_out:
  spe_epoll_disable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  conn->readType  = SPE_CONN_READNONE;
  conn->writeType = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->ReadCallback.Handler);
}

static void
connect_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->readTask.Handler = SPE_HANDLER1(connect_generic, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE, &conn->readTask);
  if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
}

/*
===================================================================================================
spe_conn_connect
===================================================================================================
*/
bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE && conn->writeType == SPE_CONN_WRITENONE && 
      addr && port);
  // gen address hints
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // get address info into servinfo
  struct addrinfo *servinfo;
  if (getaddrinfo(addr, port, &hints, &servinfo)) return false;
  // try the first address
  if (!servinfo) return false;
  if (connect(conn->fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      conn->readTask.Handler  = SPE_HANDLER1(connect_start, conn);
      conn->readType          = SPE_CONN_CONNECT;
      conn->writeType         = SPE_CONN_CONNECT;
      conn->ConnectTimeout    = 0;
      SpeTaskEnqueue(&conn->readTask);
      freeaddrinfo(servinfo);
      return true;
    }
    conn->Error = 1;
  }
  // (sync): connect success or failed, call handler
  SpeTaskEnqueue(&conn->ReadCallback);
  freeaddrinfo(servinfo);
  return true;
}

static void
read_generic(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->readExpireTime && conn->readTask.Timeout) {
    conn->ReadTimeout = 1;
    goto end_out;
  }
  // read data
  char buf[BUF_SIZE];
  for (;;) {
    int res = read(conn->fd, buf, BUF_SIZE);
    // read error
    if (res == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      SPE_LOG_ERR("conn error: %s", strerror(errno));
      conn->Error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->Closed = 1;
      break;
    }
    // read some data break
    spe_string_catb(conn->readBuffer, buf, res);
    break;
  }
  // check read type
  if (conn->readType == SPE_CONN_READUNTIL) {
    int pos = spe_string_search(conn->readBuffer, conn->_delim);
    if (pos != -1) {
      spe_string_copyb(conn->Buffer, conn->readBuffer->data, pos);
      spe_string_consume(conn->readBuffer, pos + strlen(conn->_delim));
      goto end_out;
    }
  } else if (conn->readType == SPE_CONN_READBYTES) {
    if (conn->_rbytes <= conn->readBuffer->len) {
      spe_string_copyb(conn->Buffer, conn->readBuffer->data, conn->_rbytes);
      spe_string_consume(conn->readBuffer, conn->_rbytes);
      goto end_out;
    }
  } else if (conn->readType == SPE_CONN_READ) {
    if (conn->readBuffer->len > 0) { 
      spe_string_copyb(conn->Buffer, conn->readBuffer->data, conn->readBuffer->len);
      spe_string_consume(conn->readBuffer, conn->readBuffer->len);
      goto end_out;
    }
  }
  // check error and close 
  if (conn->Closed || conn->Error) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->fd, SPE_EPOLL_READ);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  conn->readType = SPE_CONN_READNONE;
  SPE_HANDLER_CALL(conn->ReadCallback.Handler);
}

static void
read_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->readTask.Handler = SPE_HANDLER1(read_generic, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_READ, &conn->readTask);
  if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
}

/*
===================================================================================================
spe_conn_read_until
===================================================================================================
*/
bool
spe_conn_readuntil(spe_conn_t* conn, char* delim) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (!delim || conn->Closed || conn->Error) return false;
  // (sync):
  int pos = spe_string_search(conn->readBuffer, delim);
  if (pos != -1) {
    spe_string_copyb(conn->Buffer, conn->readBuffer->data, pos);
    spe_string_consume(conn->readBuffer, pos + strlen(delim));
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(read_start, conn);
  conn->ReadTimeout        = 0;
  conn->_delim              = delim;
  conn->readType          = SPE_CONN_READUNTIL;
  SpeTaskEnqueue(&conn->readTask);
  return true;
}

/*
===================================================================================================
spe_conn_readbytes
===================================================================================================
*/
bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (len == 0 || conn->Closed || conn->Error ) return false;
  // (sync):
  if (len <= conn->readBuffer->len) {
    spe_string_copyb(conn->Buffer, conn->readBuffer->data, len);
    spe_string_consume(conn->readBuffer, len);
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(read_start, conn);
  conn->ReadTimeout        = 0;
  conn->_rbytes             = len;
  conn->readType          = SPE_CONN_READBYTES;
  SpeTaskEnqueue(&conn->readTask);
  return true;
}

/*
===================================================================================================
spe_conn_read
===================================================================================================
*/
bool
spe_conn_read(spe_conn_t* conn) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (conn->Closed || conn->Error) return false;
  // (sync):
  if (conn->readBuffer->len > 0) {
    spe_string_copyb(conn->Buffer, conn->readBuffer->data, conn->readBuffer->len);
    spe_string_consume(conn->readBuffer, conn->readBuffer->len);
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(read_start, conn);
  conn->ReadTimeout       = 0;
  conn->readType          = SPE_CONN_READ;
  SpeTaskEnqueue(&conn->readTask);
  return true;
}

static void
write_generic(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->writeExpireTime && conn->writeTask.Timeout) {
    conn->WriteTimeout = 1;
    goto end_out;
  }
  // write date
  int res = write(conn->fd, conn->writeBuffer->data, conn->writeBuffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->Closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->Error = 1;
    }
    goto end_out;
  }
  spe_string_consume(conn->writeBuffer, res);
  if (conn->writeBuffer->len == 0) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->fd, SPE_EPOLL_WRITE);
  if (conn->writeExpireTime) SpeTaskDequeueTimer(&conn->writeTask);
  conn->writeType = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->WriteCallback.Handler);
}

static void
write_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->writeTask.Handler = SPE_HANDLER1(write_generic, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_WRITE, &conn->writeTask);
  if (conn->writeExpireTime) SpeTaskEnqueueTimer(&conn->writeTask, conn->writeExpireTime);
}

/*
===================================================================================================
spe_conn_flush
===================================================================================================
*/
bool
spe_conn_flush(spe_conn_t* conn) {
  ASSERT(conn && conn->writeType == SPE_CONN_WRITENONE);
  if (conn->Closed || conn->Error) return false;
  if (conn->writeBuffer->len == 0) {
    SpeTaskEnqueue(&conn->WriteCallback);
    return true;
  }
  conn->writeTask.Handler  = SPE_HANDLER1(write_start, conn);
  conn->writeType          = SPE_CONN_WRITE;
  SpeTaskEnqueue(&conn->writeTask);
  return true;
}

/*
===================================================================================================
spe_conn_set_timeout
===================================================================================================
*/
bool
spe_conn_set_timeout(spe_conn_t* conn, unsigned readExpireTime, unsigned writeExpireTime) {
  ASSERT(conn);
  conn->readExpireTime  = readExpireTime;
  conn->writeExpireTime = writeExpireTime;
  return true;
}

static bool
conn_init(spe_conn_t* conn, unsigned fd) {
  conn->fd = fd;
  SpeTaskInit(&conn->readTask);
  SpeTaskInit(&conn->writeTask);
  SpeTaskInit(&conn->ReadCallback);
  SpeTaskInit(&conn->WriteCallback);
  conn->readBuffer  = spe_string_create(BUF_SIZE);
  conn->writeBuffer = spe_string_create(BUF_SIZE);
  conn->Buffer      = spe_string_create(BUF_SIZE);
  if (!conn->readBuffer || !conn->writeBuffer || !conn->Buffer) {
    spe_string_destroy(conn->readBuffer);
    spe_string_destroy(conn->writeBuffer);
    spe_string_destroy(conn->Buffer);
    return false;
  }
  conn->_init = 1;
  return true;
}

/*
===================================================================================================
spe_conn_create
===================================================================================================
*/
spe_conn_t*
spe_conn_create(unsigned fd) {
  if (unlikely(fd >= MAX_FD)) return NULL;
  spe_sock_set_block(fd, 0);
  spe_conn_t* conn = &all_conn[fd];
  if (!conn->_init && !conn_init(conn, fd)) {
    SPE_LOG_ERR("conn_init error");
    return NULL;
  }
  spe_string_clean(conn->readBuffer);
  spe_string_clean(conn->writeBuffer);
  // init conn status
  conn->readExpireTime  = 0;
  conn->writeExpireTime = 0;
  conn->readType        = SPE_CONN_READNONE;
  conn->writeType       = SPE_CONN_WRITENONE;
  conn->Closed          = 0;
  conn->Error           = 0;
  return conn;
}

static void
spe_conn_destroy_generic(void* arg) {
  spe_conn_t* conn = arg;
  SpeTaskDequeue(&conn->readTask);
  SpeTaskDequeue(&conn->writeTask);
  SpeTaskDequeue(&conn->ReadCallback);
  SpeTaskDequeue(&conn->WriteCallback);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  if (conn->writeExpireTime) SpeTaskDequeueTimer(&conn->writeTask);
  spe_epoll_disable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  spe_sock_close(conn->fd);
}

/*
===================================================================================================
spe_conn_destroy
===================================================================================================
*/
void
spe_conn_destroy(spe_conn_t* conn) {
  ASSERT(conn);
  conn->readTask.Handler = SPE_HANDLER1(spe_conn_destroy_generic, conn);
  SpeTaskEnqueue(&conn->readTask);
}
