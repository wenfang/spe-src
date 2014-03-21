#ifndef __SPE_EPOLL_H
#define __SPE_EPOLL_H

#include "spe_handler.h"
#include <stdbool.h>

#define SPE_EPOLL_NONE   0
#define SPE_EPOLL_LISTEN 1
#define SPE_EPOLL_READ   1
#define SPE_EPOLL_WRITE  2

struct spe_epoll_s {
  spe_handler_t read_handler;
  spe_handler_t write_handler;
  unsigned      mask:2;             // mask set in epoll
};
typedef struct spe_epoll_s spe_epoll_t;

extern bool
spe_epoll_enable(unsigned fd, unsigned mask, spe_handler_t handler);

extern bool
spe_epoll_disable(unsigned fd, unsigned mask);

extern void
spe_epoll_process(int timeout);

extern void
spe_epoll_wakeup(void);

#endif
