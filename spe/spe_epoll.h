#ifndef __SPE_EPOLL_H
#define __SPE_EPOLL_H

#include "spe_task.h"
#include <stdbool.h>

#define SPE_EPOLL_NONE   0
#define SPE_EPOLL_LISTEN 1
#define SPE_EPOLL_READ   1
#define SPE_EPOLL_WRITE  2

extern bool
epollEnable(unsigned fd, unsigned mask, SpeTask_t* task);

extern bool
epollDisable(unsigned fd, unsigned mask);

extern void
epollProcess(int timeout);

extern void
epollWakeup(void);

extern void
epollFork(void);

#endif
