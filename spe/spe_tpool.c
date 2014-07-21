#include "spe_tpool.h"
#include "spe_util.h"
#include "spe_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREAD  64

struct spe_thread_s {
  pthread_t         id;
  pthread_mutex_t   lock;
  pthread_cond_t    ready;
  spe_handler_t     handler;
  struct list_head  freeListNode;
  unsigned          stop:1;
};
typedef struct spe_thread_s spe_thread_t;

struct spe_tpool_s {
  unsigned          threadNum;
  unsigned          threadRun;
  struct list_head  freeList;
  pthread_mutex_t   freeListLock;
  spe_thread_t      threads[MAX_THREAD];
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
  SPE_HANDLER_CALL(thread->handler);
  // quit thread
  __sync_sub_and_fetch(&gTpool->threadRun, 1);
  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
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
    pthread_mutex_lock(&thread->lock);
    while (!thread->handler.handler && !thread->stop) {
      pthread_cond_wait(&thread->ready, &thread->lock);
    }
    pthread_mutex_unlock(&thread->lock);
    if (unlikely(thread->stop)) break;
    // run handler
    SPE_HANDLER_CALL(thread->handler);
    thread->handler = SPE_HANDLER_NULL;
    // add to free list
    pthread_mutex_lock(&gTpool->freeListLock);
    list_add_tail(&thread->freeListNode, &gTpool->freeList);
    pthread_mutex_unlock(&gTpool->freeListLock);
  }
  // quit thread
  __sync_sub_and_fetch(&gTpool->threadRun, 1);
  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
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
  pthread_mutex_init(&thread->lock, NULL);
  pthread_cond_init(&thread->ready, NULL);
  thread->handler  = handler;
  thread->stop     = 0;   
  // put into all 
  __sync_add_and_fetch(&gTpool->threadRun, 1);
  // create thread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread->id, &attr, routine, thread); 
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
  if (!list_empty(&gTpool->freeList)) {
    // get free thread
    pthread_mutex_lock(&gTpool->freeListLock);
    if ((thread = list_first_entry(&gTpool->freeList, spe_thread_t, freeListNode))) {
      list_del_init(&thread->freeListNode);
    }
    pthread_mutex_unlock(&gTpool->freeListLock);
    // set handler to thread
    if (likely(thread != NULL)) {
      pthread_mutex_lock(&thread->lock);
      thread->handler = handler;
      pthread_mutex_unlock(&thread->lock);
      pthread_cond_signal(&thread->ready);
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
  gTpool->threadNum = spe_cpu_count() * 2;
  if (gTpool->threadNum > MAX_THREAD) gTpool->threadNum = MAX_THREAD;
  INIT_LIST_HEAD(&gTpool->freeList);
  pthread_mutex_init(&gTpool->freeListLock, NULL);
  // enable thread
  for (int i=0; i<gTpool->threadNum; i++) {
    spe_thread_t* thread = &gTpool->threads[i];
    INIT_LIST_HEAD(&thread->freeListNode);
    list_add_tail(&thread->freeListNode, &gTpool->freeList);
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
  for (int i=0; i<gTpool->threadNum; i++) {
    spe_thread_t* thread = &gTpool->threads[i];
    pthread_mutex_lock(&thread->lock);
    thread->stop = 1;
    pthread_mutex_unlock(&thread->lock);
    pthread_cond_signal(&thread->ready);
  }
  // wait all quit
  int sleep_cnt = 0;
  while (sleep_cnt > 10) {
    int all = __sync_fetch_and_add(&gTpool->threadRun, 0);
    if (!all) {
      SPE_LOG_ERR("SpeTpoolDeinit wait too long");
      break;
    }
    usleep(50000);
    sleep_cnt++;
  }
  pthread_mutex_destroy(&gTpool->freeListLock);
  free(gTpool);
  gTpool = NULL;
}
