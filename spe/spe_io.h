#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_string.h"

struct spe_io_s {
  spe_string_t* Data;
  spe_string_t* readBuffer;
  spe_string_t* writeBuffer;
  const char*   delim;
  unsigned      fd;
  unsigned      rbytes;
  unsigned      rtype:2;
  unsigned      closed:1;
  unsigned      error:1;
};
typedef struct spe_io_s spe_io_t;

extern int
SpeIoRead(spe_io_t* io);

extern int  
SpeIoReaduntil(spe_io_t* io, const char* delim);

extern int  
SpeIoReadbytes(spe_io_t* io, unsigned len);

extern int
SpeIoFlush(spe_io_t* io);

static inline bool
SpeIoWrite(spe_io_t* io, spe_string_t* buf) {
  ASSERT(io && buf);
  if (io->closed || io->error) return false;
  return spe_string_catb(io->writeBuffer, buf->data, buf->len);
}

static inline bool
SpeIoWriteb(spe_io_t* io, char* buf, unsigned len) {
  ASSERT(io && buf && len);
  if (io->closed || io->error) return false;
  return spe_string_catb(io->writeBuffer, buf, len);
}

extern spe_io_t*
SpeIoCreate(const char* fname);

extern spe_io_t*
SpeIoCreateFd(int fd);

extern void
SpeIoDestroy(spe_io_t* io);

#endif
