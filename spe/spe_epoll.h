#ifndef __SPE_EPOLL_H
#define __SPE_EPOLL_H

#include "spe_task.h"
#include <stdbool.h>

#define SPE_EPOLL_NONE   0
#define SPE_EPOLL_LISTEN 1
#define SPE_EPOLL_READ   1
#define SPE_EPOLL_WRITE  2

extern bool
speEpollEnable(unsigned fd, unsigned mask, SpeTask_t* task);

extern bool
speEpollDisable(unsigned fd, unsigned mask);

extern void
speEpollProcess(int timeout);

extern void
speEpollWakeup(void);

extern void
speEpollFork();

#endif
