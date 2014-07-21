#include "spe_tpool.h"
#include "spe_util.h"
#include "spe_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREAD  64

struct spe_tpool_s;

struct spe_thread_s {
  pthread_t           _id;
  pthread_mutex_t     _lock;
  pthread_cond_t      _ready;
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

static spe_tpool_t* gTpool;

typedef void* (*threadRoutine_Handler)(void*);


/*
===================================================================================================
threadRoutineOnce
===================================================================================================
*/
static void*
threadRoutineOnce(void* arg) {
  spe_thread_t* thread = arg;
  // run thread
  SPE_HANDLER_CALL(thread->_handler);
  // quit thread
  __sync_sub_and_fetch(&gTpool->_nall, 1);
  pthread_mutex_destroy(&thread->_lock);
  pthread_cond_destroy(&thread->_ready);
  free(thread);
  return NULL;
}

/*
===================================================================================================
threadRoutine
===================================================================================================
*/
static void*
threadRoutine(void* arg) {
  spe_thread_t* thread = arg;
  // run thread
  while (1) {
    pthread_mutex_lock(&thread->_lock);
    while (!thread->_handler.handler && !thread->_stop) {
      pthread_cond_wait(&thread->_ready, &thread->_lock);
    }
    pthread_mutex_unlock(&thread->_lock);
    if (unlikely(thread->_stop)) break;
    // run handler
    SPE_HANDLER_CALL(thread->_handler);
    thread->_handler = SPE_HANDLER_NULL;
    // add to free list
    pthread_mutex_lock(&gTpool->_free_lock);
    list_add_tail(&thread->_free_node, &gTpool->_free);
    pthread_mutex_unlock(&gTpool->_free_lock);
  }
  // quit thread
  __sync_sub_and_fetch(&gTpool->_nall, 1);
  pthread_mutex_destroy(&thread->_lock);
  pthread_cond_destroy(&thread->_ready);
  free(thread);
  return NULL;
}

/*
===================================================================================================
threadInit
===================================================================================================
*/
static void
threadInit(spe_thread_t* thread, threadRoutine_Handler routine, spe_handler_t handler) {
  pthread_mutex_init(&thread->_lock, NULL);
  pthread_cond_init(&thread->_ready, NULL);
  thread->_handler  = handler;
  thread->_stop     = 0;   
  // put into all 
  __sync_add_and_fetch(&gTpool->_nall, 1);
  // create thread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread->_id, &attr, routine, thread); 
  pthread_attr_destroy(&attr);
}

/*
===================================================================================================
SpeTpoolDo
===================================================================================================
*/
bool
SpeTpoolDo(spe_handler_t handler) {
  if (!gTpool) return false;
  spe_thread_t* thread = NULL;
  if (!list_empty(&gTpool->_free)) {
    // get free thread
    pthread_mutex_lock(&gTpool->_free_lock);
    if ((thread = list_first_entry(&gTpool->_free, spe_thread_t, _free_node))) {
      list_del_init(&thread->_free_node);
    }
    pthread_mutex_unlock(&gTpool->_free_lock);
    // set handler to thread
    if (likely(thread != NULL)) {
      pthread_mutex_lock(&thread->_lock);
      thread->_handler = handler;
      pthread_mutex_unlock(&thread->_lock);
      pthread_cond_signal(&thread->_ready);
      return true;
    }
  }
  thread = calloc(1, sizeof(spe_thread_t));
  if (!thread) {
    SPE_LOG_ERR("thread calloc error");
    return false;
  }
  threadInit(thread, threadRoutineOnce, handler);
  return true;
}

/*
===================================================================================================
SpeTpoolInit
===================================================================================================
*/
extern bool
SpeTpoolInit() {
  gTpool = calloc(1, sizeof(spe_tpool_t));
  if (!gTpool) {
    SPE_LOG_ERR("tpool calloc error");
    return false;
  }
  gTpool->_nthread = spe_cpu_count() * 2;
  if (gTpool->_nthread > MAX_THREAD) gTpool->_nthread = MAX_THREAD;
  INIT_LIST_HEAD(&gTpool->_free);
  pthread_mutex_init(&gTpool->_free_lock, NULL);
  // enable thread
  for (int i=0; i<gTpool->_nthread; i++) {
    spe_thread_t* thread = &gTpool->_threads[i];
    INIT_LIST_HEAD(&thread->_free_node);
    list_add_tail(&thread->_free_node, &gTpool->_free);
    threadInit(thread, threadRoutine, SPE_HANDLER_NULL);
  }
  return true;
}

/*
===================================================================================================
SpeTpoolDeInit
===================================================================================================
*/
void
SpeTpoolDeinit() {
  if (!gTpool) return;
  // set stop flag
  for (int i=0; i<gTpool->_nthread; i++) {
    spe_thread_t* thread = &gTpool->_threads[i];
    pthread_mutex_lock(&thread->_lock);
    thread->_stop = 1;
    pthread_mutex_unlock(&thread->_lock);
    pthread_cond_signal(&thread->_ready);
  }
  // wait all quit
  int sleep_cnt = 0;
  while (sleep_cnt > 10) {
    int all = __sync_fetch_and_add(&gTpool->_nall, 0);
    if (!all) {
      SPE_LOG_ERR("SpeTpoolDeinit wait too long");
      break;
    }
    usleep(50000);
    sleep_cnt++;
  }
  pthread_mutex_destroy(&gTpool->_free_lock);
  free(gTpool);
  gTpool = NULL;
}
