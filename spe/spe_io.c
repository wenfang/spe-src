#include "spe_io.h"
#include "spe_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_LEN 1024

#define SPE_IO_READNONE   0
#define SPE_IO_READ       1
#define SPE_IO_READBYTES  2
#define SPE_IO_READUNTIL  3

static int
spe_io_read_common(spe_io_t* io) {
  int res;
  for (;;) {
    if (io->_rtype == SPE_IO_READ) {
      if (io->_read_buffer->len > 0) {
        spe_string_copy(io->data, io->_read_buffer);
        spe_string_clean(io->_read_buffer);
        io->_rtype = SPE_IO_READNONE;
        return io->data->len;
      }
    } else if (io->_rtype == SPE_IO_READBYTES) {
      if (io->_rbytes <= io->_read_buffer->len) {
        spe_string_copyb(io->data, io->_read_buffer->data, io->_rbytes);
        spe_string_consume(io->_read_buffer, io->_rbytes);
        io->_rtype = SPE_IO_READNONE;
        return io->data->len;
      }
    } else if (io->_rtype == SPE_IO_READUNTIL) {
      int pos = spe_string_search(io->_read_buffer, io->_delim);
      if (pos != -1) {
        spe_string_copyb(io->data, io->_read_buffer->data, pos);
        spe_string_consume(io->_read_buffer, pos+strlen(io->_delim));
        io->_rtype = SPE_IO_READNONE;
        return io->data->len;
      }
    }
    res = spe_string_read_append(io->_fd, io->_read_buffer, BUF_LEN);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    if (res == 0) {
      io->_closed = 1;
      break;
    }
  }
  // read error copy data
  spe_string_copy(io->data, io->_read_buffer);
  spe_string_clean(io->_read_buffer);
  io->_rtype = SPE_IO_READNONE;
  return res;
}

/*
===================================================================================================
spe_io_readbytes
===================================================================================================
*/
int 
spe_io_readbytes(spe_io_t* io, unsigned len) {
  ASSERT(io && len);
  if (io->_closed || io->_error || io->_rtype != SPE_IO_READNONE) return -1;
  io->_rtype  = SPE_IO_READBYTES;
  io->_rbytes = len;
  return spe_io_read_common(io);
}

/*
===================================================================================================
spe_io_readline
===================================================================================================
*/
int 
spe_io_readuntil(spe_io_t* io, const char* delim) {  
  ASSERT(io && delim);
  if (io->_closed || io->_error || io->_rtype != SPE_IO_READNONE) return -1;
  io->_rtype  = SPE_IO_READUNTIL;
  io->_delim  = delim;
  return spe_io_read_common(io);
}

/*
===================================================================================================
spe_io_create
===================================================================================================
*/
spe_io_t*
spe_io_create(const char* fname) {
  ASSERT(fname);
  spe_io_t* io = calloc(1, sizeof(spe_io_t));
  if (!io) return NULL;
  if ( (io->_fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  if ( !(io->data = spe_string_create(0))) {
    spe_io_destroy(io);
    return NULL;
  }
  if ( !(io->_read_buffer = spe_string_create(0))) {
    spe_io_destroy(io);
    return NULL;
  }
  if ( !(io->_write_buffer = spe_string_create(0))) {
    spe_io_destroy(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
spe_io_destroy
===================================================================================================
*/
void 
spe_io_destroy(spe_io_t* io) {
  ASSERT(io);
  close(io->_fd);
  if (io->data) spe_string_destroy(io->data);
  if (io->_read_buffer) spe_string_destroy(io->_read_buffer);
  if (io->_write_buffer) spe_string_destroy(io->_write_buffer);
  free(io);
}
