#ifndef __SPE_UTIL_H
#define __SPE_UTIL_H

#include "spe_log.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_FD  1000000

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#ifdef DEBUG
  static inline void 
  _Assert(char* file, unsigned line) {
    fprintf(stderr, "Assert Failed [%s:%d]\n", file, line);
    abort();
  }
  #define ASSERT(x) \
    if (!(x)) _Assert(__FILE__, __LINE__)
#else
  #define ASSERT(x)
#endif


static inline unsigned long 
spe_current_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static inline unsigned
spe_cpu_count() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

static inline bool
speSetMaxOpenFiles(unsigned file_num) {
  struct rlimit r;
  r.rlim_cur = file_num;
  r.rlim_max = file_num;
  if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
    SPE_LOG_ERR("setrlimit error: %s", strerror(errno));
    return false;
  }
  return true;
}

extern int
spe_daemon();

extern int
spe_spawn(unsigned procs);

#endif
