#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_string.h"

struct spe_io_s {
  spe_string_t* data;
  spe_string_t* _read_buffer;
  spe_string_t* _write_buffer;
  const char*   _delim;
  unsigned      _rbytes;
  unsigned      _fd;
  unsigned      _rtype:2;
  unsigned      _closed:1;
  unsigned      _error:1;
};
typedef struct spe_io_s spe_io_t;

extern int
spe_io_read(spe_io_t* io);

extern int  
spe_io_readuntil(spe_io_t* io, const char* delom);

extern int  
spe_io_readbytes(spe_io_t* io, unsigned len);

extern int
spe_io_flush(spe_io_t* io);

extern spe_io_t*
spe_io_create(const char* fname);

extern spe_io_t*
spe_io_create_fd(int fd);

extern void
spe_io_destroy(spe_io_t* io);

#endif
