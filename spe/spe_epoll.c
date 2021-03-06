#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

struct spe_epoll_s {
  SpeTask_t*  readTask;
  SpeTask_t*  writeTask;
  unsigned    mask:2;             // mask set in epoll
};
typedef struct spe_epoll_s spe_epoll_t;

static int          epfd;
static int          epoll_eventfd;
static spe_epoll_t  all_epoll[MAX_FD];

/*
===================================================================================================
epollChange
===================================================================================================
*/
static bool
epollChange(unsigned fd, spe_epoll_t* epoll_t, unsigned newmask) {
  if (epoll_t->mask == newmask) return true;
  // set epoll_event 
  struct epoll_event ee;
  ee.data.u64 = 0;
  ee.data.fd  = fd;
  ee.events   = 0;
  if (newmask & SPE_EPOLL_READ) ee.events |= EPOLLIN;
  if (newmask & SPE_EPOLL_WRITE) ee.events |= EPOLLOUT;
  // set op type 
  int op = EPOLL_CTL_MOD; 
  if (epoll_t->mask == SPE_EPOLL_NONE) {
    op = EPOLL_CTL_ADD;
  } else if (newmask == SPE_EPOLL_NONE) {
    op = EPOLL_CTL_DEL; 
  }
  if (epoll_ctl(epfd, op, fd, &ee) == -1) {
    SPE_LOG_ERR("epoll_ctl error: fd %d, %s", fd, strerror(errno));
    return false;
  }
  epoll_t->mask = newmask;
  return true;
}

/*
===================================================================================================
epollEnable
===================================================================================================
*/
bool
epollEnable(unsigned fd, unsigned mask, SpeTask_t* task) {
  ASSERT(task);
  if (fd >= MAX_FD) return false;
  spe_epoll_t* epoll_t = &all_epoll[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->readTask = task;
  if (mask & SPE_EPOLL_WRITE) epoll_t->writeTask = task;
  return epollChange(fd, epoll_t, epoll_t->mask | mask);
}

/*
===================================================================================================
epollDisable
===================================================================================================
*/
bool
epollDisable(unsigned fd, unsigned mask) {
  if (fd >= MAX_FD) return false;
  spe_epoll_t* epoll_t = &all_epoll[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->readTask = NULL;
  if (mask & SPE_EPOLL_WRITE) epoll_t->writeTask = NULL;
  return epollChange(fd, epoll_t, epoll_t->mask & (~mask));
}

static struct epoll_event epEvents[MAX_FD];
/*
===================================================================================================
epollProcess
===================================================================================================
*/
void
epollProcess(int timeout) {
  int events_n = epoll_wait(epfd, epEvents, MAX_FD, timeout);
  if (unlikely(events_n < 0)) {
    if (errno == EINTR) return;
    SPE_LOG_ERR("epoll_wait error: %s", strerror(errno));
    return;
  }
  if (events_n == 0) return;
  // check events
  for (int i=0; i<events_n; i++) {
    struct epoll_event* e = &epEvents[i];
    if (e->data.fd == epoll_eventfd) {
      uint64_t u;
      read(epoll_eventfd, &u, sizeof(uint64_t));
      continue;
    }
    spe_epoll_t* epoll_t = &all_epoll[e->data.fd];
    if ((e->events & EPOLLIN) && (epoll_t->mask & SPE_EPOLL_READ)) {
      SpeTaskEnqueue(epoll_t->readTask);
    }
    if ((e->events & EPOLLOUT) && (epoll_t->mask & SPE_EPOLL_WRITE)) {
      SpeTaskEnqueue(epoll_t->writeTask);
    }
  }
}

/*
===================================================================================================
epollWakeup
===================================================================================================
*/
void
epollWakeup(void) {
  uint64_t u = 1;
  if (write(epoll_eventfd, &u, sizeof(uint64_t)) <= 0) {
    SPE_LOG_ERR("epollWakeup error");
  }
}

/*
===================================================================================================
epollInit
===================================================================================================
*/
static bool
epollInit(void) {
  epfd = epoll_create(1024);
  if (epfd < 0) {
    return false;
  }
  // create eventfd
  epoll_eventfd = eventfd(0, 0);
  SpeSockSetBlock(epoll_eventfd, 0);
  // set eventfd
  struct epoll_event ee;
  ee.data.u64 = 0;
  ee.data.fd  = epoll_eventfd;
  ee.events   = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, epoll_eventfd, &ee);
  return true;
}

/*
===================================================================================================
epollFork
===================================================================================================
*/
void
epollFork(void) {
  close(epoll_eventfd);
  close(epfd);
  epollInit();
}

__attribute__((constructor))
static void
epoll_init(void) {
  epollInit();
  fprintf(stderr, "[ERROR] epoll init error\n");
  exit(0);
}
