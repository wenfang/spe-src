#ifndef __SPE_EPOLL_H
#define __SPE_EPOLL_H

#include "spe_handler.h"
#include <stdbool.h>

#define SPE_EPOLL_NONE   0
#define SPE_EPOLL_LISTEN 1
#define SPE_EPOLL_READ   1
#define SPE_EPOLL_WRITE  2

extern bool
spe_epoll_enable(unsigned fd, unsigned mask, spe_handler_t handler);

extern bool
spe_epoll_disable(unsigned fd, unsigned mask);

extern void
spe_epoll_process(int timeout);

extern void
spe_epoll_wakeup(void);

extern void
spe_epoll_fork();

#endif
