#include "spe_io.h"
#include "spe_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_LEN 4096

#define SPE_IO_READNONE   0
#define SPE_IO_READ       1
#define SPE_IO_READBYTES  2
#define SPE_IO_READUNTIL  3

/*
===================================================================================================
ioReadCommon
===================================================================================================
*/
static int
ioReadCommon(SpeIo_t* io) {
  int res;
  for (;;) {
    if (io->rtype == SPE_IO_READ) {
      if (io->readBuffer->len > 0) {
        spe_string_copy(io->Buffer, io->readBuffer);
        spe_string_clean(io->readBuffer);
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
      }
    } else if (io->rtype == SPE_IO_READBYTES) {
      if (io->rbytes <= io->readBuffer->len) {
        spe_string_copyb(io->Buffer, io->readBuffer->data, io->rbytes);
        spe_string_consume(io->readBuffer, io->rbytes);
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
      }
    } else if (io->rtype == SPE_IO_READUNTIL) {
      int pos = spe_string_search(io->readBuffer, io->delim);
      if (pos != -1) {
        spe_string_copyb(io->Buffer, io->readBuffer->data, pos);
        spe_string_consume(io->readBuffer, pos+strlen(io->delim));
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
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
  spe_string_copy(io->Buffer, io->readBuffer);
  spe_string_clean(io->readBuffer);
  io->rtype = SPE_IO_READNONE;
  return res;
}

/*
===================================================================================================
SpeIoRead
===================================================================================================
*/
int
SpeIoRead(SpeIo_t* io) {
  ASSERT(io);
  if (io->closed || io->error) return -1;
  io->rtype = SPE_IO_READ;
  return ioReadCommon(io);
}

/*
===================================================================================================
spe_io_readbytes
===================================================================================================
*/
int 
SpeIoReadbytes(SpeIo_t* io, unsigned len) {
  ASSERT(io && len);
  if (io->closed || io->error) return -1;
  io->rtype  = SPE_IO_READBYTES;
  io->rbytes = len;
  return ioReadCommon(io);
}

/*
===================================================================================================
SpeIoReaduntil
===================================================================================================
*/
int 
SpeIoReaduntil(SpeIo_t* io, const char* delim) {  
  ASSERT(io && delim);
  if (io->closed || io->error) return -1;
  io->rtype  = SPE_IO_READUNTIL;
  io->delim  = delim;
  return ioReadCommon(io);
}

/*
===================================================================================================
SpeIoFlush
===================================================================================================
*/
int
SpeIoFlush(SpeIo_t* io) {
  ASSERT(io);
  if (io->closed || io->error) return -1;
  int totalWrite = 0;
  while (totalWrite < io->writeBuffer->len) {
    int res = write(io->fd, io->writeBuffer->data+totalWrite, io->writeBuffer->len-totalWrite);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->error = 1;
      break;
    }
    totalWrite += res;
  }
  return totalWrite;
}

/*
===================================================================================================
SpeIoCreate
===================================================================================================
*/
SpeIo_t*
SpeIoCreate(const char* fname) {
  ASSERT(fname);
  SpeIo_t* io = calloc(1, sizeof(SpeIo_t));
  if (!io) return NULL;
  if ((io->fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  io->Buffer      = spe_string_create(0);
  io->readBuffer  = spe_string_create(0);
  io->writeBuffer = spe_string_create(0);
  if (!io->Buffer || !io->readBuffer || !io->writeBuffer) {
    SpeIoDestroy(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
SpeIoCreateFd
===================================================================================================
*/
SpeIo_t*
SpeIoCreateFd(int fd) {
  SpeIo_t* io = calloc(1, sizeof(SpeIo_t));
  if (!io) return NULL;
  io->fd          = fd;
  io->Buffer      = spe_string_create(0);
  io->readBuffer  = spe_string_create(0);
  io->writeBuffer = spe_string_create(0);
  if (!io->Buffer || !io->readBuffer || !io->writeBuffer) {
    spe_string_destroy(io->Buffer);
    spe_string_destroy(io->readBuffer);
    spe_string_destroy(io->writeBuffer);
    free(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
SpeIoDestroy
===================================================================================================
*/
void 
SpeIoDestroy(SpeIo_t* io) {
  ASSERT(io);
  spe_string_destroy(io->Buffer);
  spe_string_destroy(io->readBuffer);
  spe_string_destroy(io->writeBuffer);
  close(io->fd);
  free(io);
}
