#ifndef __SPE_TPOOL_H
#define __SPE_TPOOL_H

#include "spe_list.h"
#include "spe_handler.h"
#include <stdbool.h>
#include <pthread.h>

#define MAX_THREAD  64

struct spe_tpool_s;

struct spe_thread_s {
  pthread_t           _id;
  pthread_mutex_t     _lock;
  pthread_cond_t      _ready;
  struct spe_tpool_s* _tpool;
  spe_handler_t       _handler;
  struct list_head    _free_node;
  unsigned            _stop:1;
};
typedef struct spe_thread_s spe_thread_t;

struct spe_tpool_s {
  unsigned          _nthread;
  unsigned          _nall;
  struct list_head  _free;
  pthread_mutex_t   _free_lock;
  spe_thread_t      _threads[MAX_THREAD];
};
typedef struct spe_tpool_s spe_tpool_t;

extern void
spe_tpool_enable(spe_tpool_t* tpool);

extern bool
spe_tpool_do(spe_tpool_t* tpool, spe_handler_t handler);

extern spe_tpool_t*
spe_tpool_create();

extern void
spe_tpool_destroy(spe_tpool_t* tpool);

#endif
