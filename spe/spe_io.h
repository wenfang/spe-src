#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_string.h"

struct SpeIo_s {
  spe_string_t* Buffer;
  spe_string_t* readBuffer;
  spe_string_t* writeBuffer;
  const char*   delim;
  unsigned      fd;
  unsigned      rbytes;
  unsigned      rtype:2;
  unsigned      closed:1;
  unsigned      error:1;
};
typedef struct SpeIo_s SpeIo_t;

extern int
SpeIoRead(SpeIo_t* io);

extern int  
SpeIoReaduntil(SpeIo_t* io, const char* delim);

extern int  
SpeIoReadbytes(SpeIo_t* io, unsigned len);

extern int
SpeIoFlush(SpeIo_t* io);

static inline bool
SpeIoWrite(SpeIo_t* io, spe_string_t* buf) {
  ASSERT(io && buf);
  if (io->closed || io->error) return false;
  return spe_string_catb(io->writeBuffer, buf->data, buf->len);
}

static inline bool
SpeIoWriteb(SpeIo_t* io, char* buf, unsigned len) {
  ASSERT(io && buf && len);
  if (io->closed || io->error) return false;
  return spe_string_catb(io->writeBuffer, buf, len);
}

static inline bool
SpeIoWrites(SpeIo_t* io, char* buf) {
  ASSERT(io && buf);
  if (io->closed || io->error) return false;
  return spe_string_catb(io->writeBuffer, buf, strlen(buf));
}

extern SpeIo_t*
SpeIoCreate(const char* fname);

extern SpeIo_t*
SpeIoCreateFd(int fd);

extern void
SpeIoDestroy(SpeIo_t* io);

#endif
