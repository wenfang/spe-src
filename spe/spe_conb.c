#include "spe_conb.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#define SPE_CONB_READNONE   0
#define SPE_CONB_READ       1
#define SPE_CONB_READBYTES  2 
#define SPE_CONB_READUNTIL  3

#define BUF_SIZE            1024

/*
===================================================================================================
spe_conb_read_common
===================================================================================================
*/
static int 
spe_conb_read_common(spe_conb_t* conn, spe_string_t* data) {
  int ret = SPE_CONB_INNER;
  char buf[BUF_SIZE];
  // read data first in a loop
  for (;;) {
    // buffer has enough data, copy and return
    if (conn->rtype == SPE_CONB_READBYTES) {
      if (conn->rbytes <= conn->readBuffer->len) { 
        spe_string_copyb(data, conn->readBuffer->data, conn->rbytes);
        spe_string_consume(conn->readBuffer, conn->rbytes);
        conn->rtype = SPE_CONB_READNONE;
        return data->len;
      }
    } else if (conn->rtype == SPE_CONB_READUNTIL) {
      int pos = spe_string_search(conn->readBuffer, conn->delim);
      if (pos != -1) {
        spe_string_copyb(data, conn->readBuffer->data, pos);
        spe_string_consume(conn->readBuffer, pos + strlen(conn->delim));
        conn->rtype = SPE_CONB_READNONE;
        return data->len;
      }
    } else if (conn->rtype == SPE_CONB_READ) {
      if (conn->readBuffer->len > 0) {
        spe_string_copy(data, conn->readBuffer);
        spe_string_clean(conn->readBuffer);
        conn->rtype = SPE_CONB_READNONE;
        return data->len;
      }
    }
    // we must read data  
    ret = read(conn->fd, buf, BUF_SIZE);
    if (ret > 0) {
      spe_string_catb(conn->readBuffer, buf, ret);
      continue;
    }
    if (ret == SPE_CONB_ERROR) {           
      if (errno == EINTR) continue;           
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        ret = SPE_CONB_TIMEOUT;
        break;
      }
    }
    // peer close or not eagain
    conn->closed = true;
    break;
  }
  // read error or read close, copy data
  spe_string_copy(data, conn->readBuffer);
  spe_string_clean(conn->readBuffer);
  conn->rtype = SPE_CONB_READNONE;
  return ret;
}

/*
===================================================================================================
spe_conb_read
  read data normal
===================================================================================================
*/
int 
spe_conb_read(spe_conb_t* conn, spe_string_t* buf) {
  ASSERT(conn && buf);
  if (conn->closed || conn->error) return SPE_CONB_INNER;
  conn->rtype = SPE_CONB_READ;
  return spe_conb_read_common(conn, buf);
}

/* 
===================================================================================================
spe_conb_readbytes
  read len size bytes 
===================================================================================================
*/
int 
spe_conb_readbytes(spe_conb_t* conn, unsigned len, spe_string_t* buf) {
  ASSERT(conn && len && buf);
  if (conn->closed || conn->error) return SPE_CONB_INNER;
  conn->rtype  = SPE_CONB_READBYTES;    
  conn->rbytes = len;
  return spe_conb_read_common(conn, buf);
}

/* 
===================================================================================================
spe_conb_readuntil
===================================================================================================
*/
int 
spe_conb_readuntil(spe_conb_t* conn, const char* delim, spe_string_t* buf) {
  ASSERT(conn && buf && delim);
  if (conn->closed || conn->error) return SPE_CONB_INNER;
  conn->rtype  = SPE_CONB_READUNTIL;
  conn->delim  = delim;
  return spe_conb_read_common(conn, buf);
}

/*
===================================================================================================
spe_conb_writeb
  write data to conn
  return: -1 --- write error
      -2 --- write timeout
      other --- write data
===================================================================================================
*/
int 
spe_conb_writeb(spe_conb_t* conn, char *buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->closed || conn->error) return SPE_CONB_INNER;
  int total_write = 0;
  while (total_write < len) {
    int res = write(conn->fd, buf + total_write, len);
    if (res == SPE_CONB_ERROR) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) return SPE_CONB_TIMEOUT;
      conn->error = true;
      return res;
    }
    total_write += res;
  }
  return total_write;
}

/*
===================================================================================================
spe_conb_set_timeout
===================================================================================================
*/
bool 
spe_conb_set_timeout(spe_conb_t* conn, unsigned read_timeout, unsigned write_timeout) {
  ASSERT(conn);
  struct timeval tv;
  if (read_timeout) {
    tv.tv_sec   = read_timeout/ 1000;
    tv.tv_usec  = (read_timeout % 1000) * 1000;
    if (setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      SPE_LOG_ERR("setsockopt error");
      return false;
    }
  }
  if (write_timeout) {
    tv.tv_sec   = write_timeout/ 1000;
    tv.tv_usec  = (write_timeout % 1000) * 1000;
    if (setsockopt(conn->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
      SPE_LOG_ERR("setsockopt error");
      return false;
    }
  }
  return true;
}

static spe_conb_t  _conn[MAX_FD];

/*
===================================================================================================
spe_conb_create
===================================================================================================
*/
spe_conb_t*
spe_conb_create(unsigned fd) {
  if (unlikely(fd >= MAX_FD)) {
    SPE_LOG_ERR("fd is too large");
    return NULL;
  }
  if (unlikely(!spe_sock_set_block(fd, 1))) {
    SPE_LOG_ERR("spe_sock_set_block error");
    return NULL;
  }
  spe_conb_t* conn = &_conn[fd];
  conn->fd = fd;      
  if (!conn->readBuffer && !(conn->readBuffer = spe_string_create(BUF_SIZE))) {
    SPE_LOG_ERR("read buffer create error");
    return NULL;
  }
  spe_string_clean(conn->readBuffer);
  // init read
  conn->rtype  = SPE_CONB_READNONE;
  conn->delim  = NULL;
  conn->rbytes = 0;
  conn->closed = 0;
  conn->error  = 0;
  return conn;
}

/*
===================================================================================================
spe_conb_destroy
===================================================================================================
*/
void
spe_conb_destroy(spe_conb_t* conn) {
  ASSERT(conn);
  spe_sock_close(conn->fd);
}

/*
===================================================================================================
spe_conb_connect_wait
===================================================================================================
*/
static bool
spe_conb_connect_wait(int fd, unsigned timeout) {
  struct pollfd wfd[1];
  wfd[0].fd     = fd; 
  wfd[0].events = POLLOUT;
  // set connect timeout value
  long msc = -1;
  if (timeout) msc = timeout;
  // pool for wait
  if (poll(wfd, 1, msc) <= 0) {
    SPE_LOG_ERR("pool error");
    return false;
  }
  return true;
}

/*
===================================================================================================
spe_conb_connect
  conn to addr and port
  return NULL -- fail, other -- success
===================================================================================================
*/
spe_conb_t*
spe_conb_connect(const char *addr, const char* port, unsigned timeout) {
  ASSERT(addr && port);
  // gen address hints
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // get address info into servinfo
  struct addrinfo *servinfo;
  if (getaddrinfo(addr, port, &hints, &servinfo) != 0) {
    SPE_LOG_ERR("getaddrinfo error");
    return NULL;
  }
  // for each address
  struct addrinfo *p;
  for (p=servinfo; p!=NULL; p=p->ai_next) {
    int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd == -1) continue;
    if (!spe_sock_set_block(fd, 0)) continue;
    // try to connect
    if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
      if (errno != EINPROGRESS) {
        SPE_LOG_ERR("conn error");
        spe_sock_close(fd);
        continue;
      }
      // now, errno == EINPROGRESS
      if (!spe_conb_connect_wait(fd, timeout)) {
        spe_sock_close(fd);
        continue;
      }
    } 
    // connect ok return spe_conb_t
    freeaddrinfo(servinfo);
    return spe_conb_create(fd);
  }
  freeaddrinfo(servinfo);
  return NULL;
}
