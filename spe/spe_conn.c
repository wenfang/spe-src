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
#define SPE_CONN_CONNECT    1
#define SPE_CONN_WRITE      1

static spe_conn_t all_conn[MAX_FD];

static void
connect_end(spe_conn_t* conn) {
  spe_epoll_disable(conn->fd, SPE_EPOLL_WRITE);
  if (conn->write_expire_time) spe_timer_disable(&conn->write_task);
  conn->write_type = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->write_callback_task.handler);
}

static void
connect_common(void* arg) {
  spe_conn_t* conn = arg;
  // connect timeout
  if (conn->write_expire_time && conn->write_task.timeout) {
    conn->connect_timeout = 1;
    connect_end(conn);
    return;
  }
  // check connect timeout
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->error = 1;
  if (err) conn->error = 1;
  connect_end(conn);
}

static void
connect_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->write_task.handler = SPE_HANDLER1(connect_common, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_WRITE, &conn->write_task);
  if (conn->write_expire_time) spe_timer_enable(&conn->write_task, conn->write_expire_time);
}

/*
===================================================================================================
spe_conn_connect
===================================================================================================
*/
bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port, spe_handler_t handler) {
  ASSERT(conn && conn->write_type == SPE_CONN_WRITENONE && addr && port);
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
  conn->write_callback_task.handler = handler;
  if (connect(conn->fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      conn->write_task.handler  = SPE_HANDLER1(connect_start, conn);
      conn->write_type          = SPE_CONN_CONNECT;
      conn->connect_timeout     = 0;
      connect_start(conn);
      //spe_task_enqueue(&conn->write_task);
      freeaddrinfo(servinfo);
      return true;
    }
    conn->error = 1;
  }
  // (sync): connect success or failed, call handler
  spe_task_enqueue(&conn->write_callback_task);
  freeaddrinfo(servinfo);
  return true;
}

static void
read_end(spe_conn_t* conn) {
  spe_epoll_disable(conn->fd, SPE_EPOLL_READ);
  if (conn->read_expire_time) spe_timer_disable(&conn->read_task);
  conn->read_type = SPE_CONN_READNONE;
  SPE_HANDLER_CALL(conn->read_callback_task.handler);
}

static void
read_common(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->read_expire_time && conn->read_task.timeout) {
    conn->read_timeout = 1;
    read_end(conn);
    return;
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
      conn->error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->closed = 1;
      break;
    }
    // read some data break
    spe_string_catb(conn->read_buffer, buf, res);
    break;
  }
  // check read type
  if (conn->read_type == SPE_CONN_READUNTIL) {
    int pos = spe_string_search(conn->read_buffer, conn->delim);
    if (pos != -1) {
      spe_string_copyb(conn->buffer, conn->read_buffer->data, pos);
      spe_string_consume(conn->read_buffer, pos + strlen(conn->delim));
      read_end(conn);
      return;
    }
  } else if (conn->read_type == SPE_CONN_READBYTES) {
    if (conn->rbytes <= conn->read_buffer->len) {
      spe_string_copyb(conn->buffer, conn->read_buffer->data, conn->rbytes);
      spe_string_consume(conn->read_buffer, conn->rbytes);
      read_end(conn);
      return;
    }
  } else if (conn->read_type == SPE_CONN_READ) {
    if (conn->read_buffer->len > 0) { 
      spe_string_copyb(conn->buffer, conn->read_buffer->data, conn->read_buffer->len);
      spe_string_consume(conn->read_buffer, conn->read_buffer->len);
      read_end(conn);
      return;
    }
  }
  // check error and close 
  if (conn->closed || conn->error) read_end(conn);
}

static void
read_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->read_task.handler = SPE_HANDLER1(read_common, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_READ, &conn->read_task);
  if (conn->read_expire_time) spe_timer_enable(&conn->read_task, conn->read_expire_time);
}

/*
===================================================================================================
spe_conn_read_until
===================================================================================================
*/
bool
spe_conn_readuntil(spe_conn_t* conn, char* delim, spe_handler_t handler) {
  ASSERT(conn && conn->read_type == SPE_CONN_READNONE);
  if (!delim || conn->closed || conn->error) return false;
  conn->read_callback_task.handler = handler;
  // (sync):
  int pos = spe_string_search(conn->read_buffer, delim);
  if (pos != -1) {
    spe_string_copyb(conn->buffer, conn->read_buffer->data, pos);
    spe_string_consume(conn->read_buffer, pos + strlen(delim));
    spe_task_enqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  conn->read_task.handler = SPE_HANDLER1(read_start, conn);
  conn->read_timeout      = 0;
  conn->delim             = delim;
  conn->read_type         = SPE_CONN_READUNTIL;
  read_start(conn);
  //spe_task_enqueue(&conn->read_task);
  return true;
}

/*
===================================================================================================
spe_conn_readbytes
===================================================================================================
*/
bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len, spe_handler_t handler) {
  ASSERT(conn && conn->read_type == SPE_CONN_READNONE);
  if (len == 0 || conn->closed || conn->error ) return false;
  conn->read_callback_task.handler = handler;
  // (sync):
  if (len <= conn->read_buffer->len) {
    spe_string_copyb(conn->buffer, conn->read_buffer->data, len);
    spe_string_consume(conn->read_buffer, len);
    spe_task_enqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  // conn->read_task.handler = SPE_HANDLER1(read_start, conn);
  conn->read_timeout      = 0;
  conn->rbytes            = len;
  conn->read_type         = SPE_CONN_READBYTES;
  read_start(conn);
  //spe_task_enqueue(&conn->read_task);
  return true;
}

/*
===================================================================================================
spe_conn_read
===================================================================================================
*/
bool
spe_conn_read(spe_conn_t* conn, spe_handler_t handler) {
  ASSERT(conn && conn->read_type == SPE_CONN_READNONE);
  if (conn->closed || conn->error) return false;
  conn->read_callback_task.handler = handler;
  // (sync):
  if (conn->read_buffer->len > 0) {
    spe_string_copyb(conn->buffer, conn->read_buffer->data, conn->read_buffer->len);
    spe_string_consume(conn->read_buffer, conn->read_buffer->len);
    spe_task_enqueue(&conn->read_callback_task);
    return true;
  }
  // (async):
  //conn->read_task.handler = SPE_HANDLER1(read_start, conn);
  conn->read_timeout      = 0;
  conn->read_type         = SPE_CONN_READ;
  read_start(conn);
  //spe_task_enqueue(&conn->read_task);
  return true;
}

static void
write_end(spe_conn_t* conn) {
  spe_epoll_disable(conn->fd, SPE_EPOLL_WRITE);
  if (conn->write_expire_time) spe_timer_disable(&conn->write_task);
  conn->write_type = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->write_callback_task.handler);
}

static void
write_common(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->write_expire_time && conn->write_task.timeout) {
    conn->write_timeout = 1;
    write_end(conn);
    return;
  }
  // write date
  int res = write(conn->fd, conn->write_buffer->data, conn->write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    write_end(conn);
    return;
  }
  spe_string_consume(conn->write_buffer, res);
  if (conn->write_buffer->len == 0) write_end(conn);
}

static void
write_start(void* arg) {
  spe_conn_t* conn = arg;
  conn->write_task.handler = SPE_HANDLER1(write_common, conn);
  spe_epoll_enable(conn->fd, SPE_EPOLL_WRITE, &conn->write_task);
  if (conn->write_expire_time) spe_timer_enable(&conn->write_task, conn->write_expire_time);
}

/*
===================================================================================================
spe_conn_flush
===================================================================================================
*/
bool
spe_conn_flush(spe_conn_t* conn, spe_handler_t handler) {
  ASSERT(conn && conn->write_type == SPE_CONN_WRITENONE);
  if (conn->closed || conn->error) return false;
  conn->write_callback_task.handler = handler;
  if (conn->write_buffer->len == 0) {
    spe_task_enqueue(&conn->write_callback_task);
    return true;
  }
  //conn->write_task.handler  = SPE_HANDLER1(write_start, conn);
  conn->write_type          = SPE_CONN_WRITE;
  write_start(conn);
  //spe_task_enqueue(&conn->write_task);
  return true;
}

/*
===================================================================================================
spe_conn_set_timeout
===================================================================================================
*/
bool
spe_conn_set_timeout(spe_conn_t* conn, unsigned read_expire_time, unsigned write_expire_time) {
  ASSERT(conn);
  conn->read_expire_time = read_expire_time;
  conn->write_expire_time = write_expire_time;
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
  spe_conn_t* conn = &all_conn[fd];
  spe_sock_set_block(fd, 0);
  conn->fd = fd;
  spe_task_init(&conn->read_task);
  spe_task_init(&conn->write_task);
  spe_task_init(&conn->read_callback_task);
  spe_task_init(&conn->write_callback_task);
  // init buffer
  if (!conn->read_buffer && !(conn->read_buffer = spe_string_create(BUF_SIZE))) {
    SPE_LOG_ERR("spe_string_create error");
    return NULL;
  }
  if (!conn->write_buffer && !(conn->write_buffer = spe_string_create(BUF_SIZE))) {
    SPE_LOG_ERR("spe_string_create error");
    return NULL;
  }
  if (!conn->buffer && !(conn->buffer = spe_string_create(BUF_SIZE))) {
    SPE_LOG_ERR("spe_string_create error");
    return NULL;
  }
  spe_string_clean(conn->read_buffer);
  spe_string_clean(conn->write_buffer);
  spe_string_clean(conn->buffer);
  // init conn status
  conn->read_expire_time  = 0;
  conn->write_expire_time = 0;
  conn->read_type         = SPE_CONN_READNONE;
  conn->write_type        = SPE_CONN_WRITENONE;
  conn->connect_timeout   = 0;
  conn->read_timeout      = 0;
  conn->write_timeout     = 0;
  conn->closed            = 0;
  conn->error             = 0;
  return conn;
}

static void
spe_conn_destroy_common(void* arg) {
  spe_conn_t* conn = arg;
  spe_timer_disable(&conn->read_task);
  spe_timer_disable(&conn->write_task);
  spe_epoll_disable(conn->fd, SPE_EPOLL_READ | SPE_EPOLL_WRITE);
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
  //conn->read_callback_task.handler = SPE_HANDLER1(spe_conn_destroy_common, conn);
  //spe_task_enqueue(&conn->read_callback_task);
  spe_conn_destroy_common(conn);
}
