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
        SpeStringCopy(io->Buffer, io->readBuffer);
        SpeStringClean(io->readBuffer);
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
      }
    } else if (io->rtype == SPE_IO_READBYTES) {
      if (io->rbytes <= io->readBuffer->len) {
        SpeStringCopyb(io->Buffer, io->readBuffer->data, io->rbytes);
        SpeStringConsume(io->readBuffer, io->rbytes);
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
      }
    } else if (io->rtype == SPE_IO_READUNTIL) {
      int pos = SpeString_search(io->readBuffer, io->delim);
      if (pos != -1) {
        SpeStringCopyb(io->Buffer, io->readBuffer->data, pos);
        SpeStringConsume(io->readBuffer, pos+strlen(io->delim));
        io->rtype = SPE_IO_READNONE;
        return io->Buffer->len;
      }
    }
    res = SpeStringread_append(io->fd, io->readBuffer, BUF_LEN);
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
  SpeStringCopy(io->Buffer, io->readBuffer);
  SpeStringClean(io->readBuffer);
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
  io->Buffer      = SpeStringCreate(0);
  io->readBuffer  = SpeStringCreate(0);
  io->writeBuffer = SpeStringCreate(0);
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
  io->Buffer      = SpeStringCreate(0);
  io->readBuffer  = SpeStringCreate(0);
  io->writeBuffer = SpeStringCreate(0);
  if (!io->Buffer || !io->readBuffer || !io->writeBuffer) {
    SpeStringDestroy(io->Buffer);
    SpeStringDestroy(io->readBuffer);
    SpeStringDestroy(io->writeBuffer);
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
  SpeStringDestroy(io->Buffer);
  SpeStringDestroy(io->readBuffer);
  SpeStringDestroy(io->writeBuffer);
  close(io->fd);
  free(io);
}
