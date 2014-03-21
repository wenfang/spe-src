#ifndef __SPE_CONB_H
#define __SPE_CONB_H

#include "spe_string.h"
#include "spe_util.h"
#include <stdbool.h>

#define SPE_CONB_CLOSED   0
#define SPE_CONB_ERROR    -1
#define SPE_CONB_TIMEOUT  -2
#define SPE_CONB_INNER    -3

struct spe_conb_s {
  spe_string_t* _read_buffer; // read read buffer 
  const char*   _delim;       // the delim when readtype is READUNTIL 
  unsigned      _rbytes;      // read data size when readtype is READBYTES 
  unsigned      _fd;          // fd to control
  unsigned      _rtype:2;     // read type
  unsigned      _closed:1;    // peer closed
  unsigned      _error:1;     // peer error  
};  
typedef struct spe_conb_s spe_conb_t;

extern int    
spe_conb_read(spe_conb_t* conn, spe_string_t* buf);

extern int    
spe_conb_readbytes(spe_conb_t* conn, unsigned len, spe_string_t* buf);

extern int    
spe_conb_readuntil(spe_conb_t* conn, const char* delim, spe_string_t* buf);

extern int    
spe_conb_writeb(spe_conb_t* conn, char* buf, unsigned len);

static inline int    
spe_conb_write(spe_conb_t* conn, spe_string_t* buf) {
  ASSERT(conn && buf);
  if (!buf->len || conn->_closed || conn->_error) return -1;
  return spe_conb_writeb(conn, buf->data, buf->len);
}

static inline int    
spe_conb_writes(spe_conb_t* conn, char* buf) {
  ASSERT(conn && buf);
  if (conn->_closed || conn->_error) return -1;
  return spe_conb_writeb(conn, buf, strlen(buf));
}

extern bool   
spe_conb_set_timeout(spe_conb_t* conn, unsigned read_timeout, unsigned write_timeout);

extern spe_conb_t* 
spe_conb_connect(const char* addr, const char* port, unsigned timeout);

extern spe_conb_t*
spe_conb_create(unsigned fd);

extern void
spe_conb_destroy(spe_conb_t* conn);

#endif
