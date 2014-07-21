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
    if (io->rtype == SPE_IO_READ) {
      if (io->readBuffer->len > 0) {
        spe_string_copy(io->Data, io->readBuffer);
        spe_string_clean(io->readBuffer);
        io->rtype = SPE_IO_READNONE;
        return io->Data->len;
      }
    } else if (io->rtype == SPE_IO_READBYTES) {
      if (io->rbytes <= io->readBuffer->len) {
        spe_string_copyb(io->Data, io->readBuffer->data, io->rbytes);
        spe_string_consume(io->readBuffer, io->rbytes);
        io->rtype = SPE_IO_READNONE;
        return io->Data->len;
      }
    } else if (io->rtype == SPE_IO_READUNTIL) {
      int pos = spe_string_search(io->readBuffer, io->delim);
      if (pos != -1) {
        spe_string_copyb(io->Data, io->readBuffer->data, pos);
        spe_string_consume(io->readBuffer, pos+strlen(io->delim));
        io->rtype = SPE_IO_READNONE;
        return io->Data->len;
      }
    }
    res = spe_string_read_append(io->fd, io->readBuffer, BUF_LEN);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->error = 1;
      break;
    }
    if (res == 0) {
      io->closed = 1;
      break;
    }
  }
  // read error copy data
  spe_string_copy(io->Data, io->readBuffer);
  spe_string_clean(io->readBuffer);
  io->rtype = SPE_IO_READNONE;
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
  if (io->closed || io->error || io->rtype != SPE_IO_READNONE) return -1;
  io->rtype  = SPE_IO_READBYTES;
  io->rbytes = len;
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
  if (io->closed || io->error || io->rtype != SPE_IO_READNONE) return -1;
  io->rtype  = SPE_IO_READUNTIL;
  io->delim  = delim;
  return spe_io_read_common(io);
}

bool
SpeIoWriteb(spe_io_t* io, char* buf, unsigned len) {
  ASSERT(io && buf && len);
  return true;
}

/*
===================================================================================================
spe_io_create
===================================================================================================
*/
spe_io_t*
SpeIoCreate(const char* fname) {
  ASSERT(fname);
  spe_io_t* io = calloc(1, sizeof(spe_io_t));
  if (!io) return NULL;
  if ( (io->fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  io->Data          = spe_string_create(0);
  io->readBuffer  = spe_string_create(0);
  io->writeBuffer = spe_string_create(0);
  if (!io->Data || !io->readBuffer || !io->writeBuffer) {
    SpeIoDestroy(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
spe_io_createfd
===================================================================================================
*/
spe_io_t*
SpeIoCreateFd(int fd) {
  spe_io_t* io = calloc(1, sizeof(spe_io_t));
  if (!io) return NULL;
  io->fd           = fd;
  io->Data          = spe_string_create(0);
  io->readBuffer  = spe_string_create(0);
  io->writeBuffer = spe_string_create(0);
  if (!io->Data || !io->readBuffer || !io->writeBuffer) {
    spe_string_destroy(io->Data);
    spe_string_destroy(io->readBuffer);
    spe_string_destroy(io->writeBuffer);
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
SpeIoDestroy(spe_io_t* io) {
  ASSERT(io);
  if (io->Data) spe_string_destroy(io->Data);
  if (io->readBuffer) spe_string_destroy(io->readBuffer);
  if (io->writeBuffer) spe_string_destroy(io->writeBuffer);
  close(io->fd);
  free(io);
}
