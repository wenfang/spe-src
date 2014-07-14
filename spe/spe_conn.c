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
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->connect_timeout = 1;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->error = 1;
  if (err) conn->error = 1;
end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  if (conn->_read_expire_time) SpeTaskDequeueTimer(&conn->_read_task);
  conn->_read_type  = SPE_CONN_READNONE;
  conn->_write_type = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->read_callback_task.handler);
}

static void
connect_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->_read_task.handler = SPE_HANDLER1(connect_generic, conn);
  spe_epoll_enable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE, &conn->_read_task);
  if (conn->_read_expire_time) SpeTaskEnqueueTimer(&conn->_read_task, conn->_read_expire_time);
}

/*
===================================================================================================
spe_conn_connect
===================================================================================================
*/
bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE && 
      conn->_write_type == SPE_CONN_WRITENONE && addr && port);
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
  if (connect(conn->_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      conn->_read_task.handler  = SPE_HANDLER1(connect_start, conn);
      conn->_read_type          = SPE_CONN_CONNECT;
      conn->_write_type         = SPE_CONN_CONNECT;
      conn->connect_timeout     = 0;
      SpeTaskEnqueue(&conn->_read_task);
      freeaddrinfo(servinfo);
      return true;
    }
    conn->error = 1;
  }
  // (sync): connect success or failed, call handler
  SpeTaskEnqueue(&conn->read_callback_task);
  freeaddrinfo(servinfo);
  return true;
}

static void
read_generic(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->read_timeout = 1;
    goto end_out;
  }
  // read data
  char buf[BUF_SIZE];
  for (;;) {
    int res = read(conn->_fd, buf, BUF_SIZE);
    // read error
    if (res == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      SPE_LOG_ERR("conn error: %s", strerror(errno));
      conn->error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->closed = 1;
      break;
    }
    // read some data break
    spe_string_catb(conn->_read_buffer, buf, res);
    break;
  }
  // check read type
  if (conn->_read_type == SPE_CONN_READUNTIL) {
    int pos = spe_string_search(conn->_read_buffer, conn->_delim);
    if (pos != -1) {
      spe_string_copyb(conn->buffer, conn->_read_buffer->data, pos);
      spe_string_consume(conn->_read_buffer, pos + strlen(conn->_delim));
      goto end_out;
    }
  } else if (conn->_read_type == SPE_CONN_READBYTES) {
    if (conn->_rbytes <= conn->_read_buffer->len) {
      spe_string_copyb(conn->buffer, conn->_read_buffer->data, conn->_rbytes);
      spe_string_consume(conn->_read_buffer, conn->_rbytes);
      goto end_out;
    }
  } else if (conn->_read_type == SPE_CONN_READ) {
    if (conn->_read_buffer->len > 0) { 
      spe_string_copyb(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
      spe_string_consume(conn->_read_buffer, conn->_read_buffer->len);
      goto end_out;
    }
  }
  // check error and close 
  if (conn->closed || conn->error) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ);
  if (conn->_read_expire_time) SpeTaskDequeueTimer(&conn->_read_task);
  conn->_read_type = SPE_CONN_READNONE;
  SPE_HANDLER_CALL(conn->read_callback_task.handler);
}

static void
read_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->_read_task.handler = SPE_HANDLER1(read_generic, conn);
  spe_epoll_enable(conn->_fd, SPE_EPOLL_READ, &conn->_read_task);
  if (conn->_read_expire_time) SpeTaskEnqueueTimer(&conn->_read_task, conn->_read_expire_time);
}

/*
===================================================================================================
spe_conn_read_until
===================================================================================================
*/
bool
spe_conn_readuntil(spe_conn_t* conn, char* delim) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (!delim || conn->closed || conn->error) return false;
  // (sync):
  int pos = spe_string_search(conn->_read_buffer, delim);
  if (pos != -1) {
    spe_string_copyb(conn->buffer, conn->_read_buffer->data, pos);
    spe_string_consume(conn->_read_buffer, pos + strlen(delim));
    SpeTaskEnqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  conn->_read_task.handler  = SPE_HANDLER1(read_start, conn);
  conn->read_timeout        = 0;
  conn->_delim              = delim;
  conn->_read_type          = SPE_CONN_READUNTIL;
  SpeTaskEnqueue(&conn->_read_task);
  return true;
}

/*
===================================================================================================
spe_conn_readbytes
===================================================================================================
*/
bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (len == 0 || conn->closed || conn->error ) return false;
  // (sync):
  if (len <= conn->_read_buffer->len) {
    spe_string_copyb(conn->buffer, conn->_read_buffer->data, len);
    spe_string_consume(conn->_read_buffer, len);
    SpeTaskEnqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  conn->_read_task.handler  = SPE_HANDLER1(read_start, conn);
  conn->read_timeout        = 0;
  conn->_rbytes             = len;
  conn->_read_type          = SPE_CONN_READBYTES;
  SpeTaskEnqueue(&conn->_read_task);
  return true;
}

/*
===================================================================================================
spe_conn_read
===================================================================================================
*/
bool
spe_conn_read(spe_conn_t* conn) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (conn->closed || conn->error) return false;
  // (sync):
  if (conn->_read_buffer->len > 0) {
    spe_string_copyb(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
    spe_string_consume(conn->_read_buffer, conn->_read_buffer->len);
    SpeTaskEnqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  conn->_read_task.handler  = SPE_HANDLER1(read_start, conn);
  conn->read_timeout        = 0;
  conn->_read_type          = SPE_CONN_READ;
  SpeTaskEnqueue(&conn->_read_task);
  return true;
}

static void
write_generic(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->_write_expire_time && conn->_write_task.timeout) {
    conn->write_timeout = 1;
    goto end_out;
  }
  // write date
  int res = write(conn->_fd, conn->_write_buffer->data, conn->_write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    goto end_out;
  }
  spe_string_consume(conn->_write_buffer, res);
  if (conn->_write_buffer->len == 0) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_WRITE);
  if (conn->_write_expire_time) SpeTaskDequeueTimer(&conn->_write_task);
  conn->_write_type = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->write_callback_task.handler);
}

static void
write_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->_write_task.handler = SPE_HANDLER1(write_generic, conn);
  spe_epoll_enable(conn->_fd, SPE_EPOLL_WRITE, &conn->_write_task);
  if (conn->_write_expire_time) SpeTaskEnqueueTimer(&conn->_write_task, conn->_write_expire_time);
}

/*
===================================================================================================
spe_conn_flush
===================================================================================================
*/
bool
spe_conn_flush(spe_conn_t* conn) {
  ASSERT(conn && conn->_write_type == SPE_CONN_WRITENONE);
  if (conn->closed || conn->error) return false;
  if (conn->_write_buffer->len == 0) {
    SpeTaskEnqueue(&conn->write_callback_task);
    return true;
  }
  conn->_write_task.handler  = SPE_HANDLER1(write_start, conn);
  conn->_write_type          = SPE_CONN_WRITE;
  SpeTaskEnqueue(&conn->_write_task);
  return true;
}

/*
===================================================================================================
spe_conn_set_timeout
===================================================================================================
*/
bool
spe_conn_set_timeout(spe_conn_t* conn, unsigned _read_expire_time, unsigned _write_expire_time) {
  ASSERT(conn);
  conn->_read_expire_time   = _read_expire_time;
  conn->_write_expire_time  = _write_expire_time;
  return true;
}

static bool
conn_init(spe_conn_t* conn, unsigned fd) {
  conn->_fd = fd;
  SpeTaskInit(&conn->_read_task);
  SpeTaskInit(&conn->_write_task);
  SpeTaskInit(&conn->read_callback_task);
  SpeTaskInit(&conn->write_callback_task);
  conn->_read_buffer  = spe_string_create(BUF_SIZE);
  conn->_write_buffer = spe_string_create(BUF_SIZE);
  conn->buffer        = spe_string_create(BUF_SIZE);
  if (!conn->_read_buffer || !conn->_write_buffer || !conn->buffer) {
    spe_string_destroy(conn->_read_buffer);
    spe_string_destroy(conn->_write_buffer);
    spe_string_destroy(conn->buffer);
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
  spe_string_clean(conn->_read_buffer);
  spe_string_clean(conn->_write_buffer);
  // init conn status
  conn->_read_expire_time   = 0;
  conn->_write_expire_time  = 0;
  conn->_read_type          = SPE_CONN_READNONE;
  conn->_write_type         = SPE_CONN_WRITENONE;
  conn->closed              = 0;
  conn->error               = 0;
  return conn;
}

static void
spe_conn_destroy_generic(void* arg) {
  spe_conn_t* conn = arg;
  SpeTaskDequeue(&conn->_read_task);
  SpeTaskDequeue(&conn->_write_task);
  SpeTaskDequeue(&conn->read_callback_task);
  SpeTaskDequeue(&conn->write_callback_task);
  if (conn->_read_expire_time) SpeTaskDequeueTimer(&conn->_read_task);
  if (conn->_write_expire_time) SpeTaskDequeueTimer(&conn->_write_task);
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  spe_sock_close(conn->_fd);
}

/*
===================================================================================================
spe_conn_destroy
===================================================================================================
*/
void
spe_conn_destroy(spe_conn_t* conn) {
  ASSERT(conn);
  conn->_read_task.handler = SPE_HANDLER1(spe_conn_destroy_generic, conn);
  SpeTaskEnqueue(&conn->_read_task);
}
