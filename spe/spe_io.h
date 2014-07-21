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
spe_io_read(spe_io_t* io);

extern int  
spe_io_readuntil(spe_io_t* io, const char* delim);

extern int  
spe_io_readbytes(spe_io_t* io, unsigned len);

extern int
spe_io_flush(spe_io_t* io);

extern int
spe_io_writeb(spe_io_t* io, char* buf, unsigned len);

extern spe_io_t*
SpeIoCreate(const char* fname);

extern spe_io_t*
SpeIoCreateFd(int fd);

extern void
SpeIoDestroy(spe_io_t* io);

#endif
