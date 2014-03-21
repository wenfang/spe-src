#include "spe_worker.h"
#include "spe_object.h"
#include "spe_util.h"
#include <pthread.h>
#include <stdlib.h>

#define MAX_WORKER  32
#define MAX_DONE    16

static LIST_HEAD(worker_queue);
static pthread_mutex_t worker_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t worker_queue_cond = PTHREAD_COND_INITIALIZER;

/*
===================================================================================================
spe_worker_do
===================================================================================================
*/
void 
spe_worker_do(spe_object_t *obj) {
  ASSERT(obj);
  pthread_mutex_lock(&worker_queue_mutex);
  if (obj->status != SPE_OBJECT_FREE) {
    pthread_mutex_unlock(&worker_queue_mutex);
    return;
  }
  list_add_tail(&obj->worker_node, &worker_queue);
  obj->status = SPE_OBJECT_QUEUE;
  pthread_mutex_unlock(&worker_queue_mutex);
  pthread_cond_signal(&worker_queue_cond);
}

/*
===================================================================================================
worker_routine
===================================================================================================
*/
static void*
worker_routine(void* unused) {
  spe_object_t* obj;
  while (1) {
    // get object from worker_queue
    pthread_mutex_lock(&worker_queue_mutex);
    while (list_empty(&worker_queue)) {
      pthread_cond_wait(&worker_queue_cond, &worker_queue_mutex);
    }
    obj = list_first_entry(&worker_queue, spe_object_t, worker_node);
    list_del_init(&obj->worker_node);
    obj->status = SPE_OBJECT_RUN;
    pthread_mutex_unlock(&worker_queue_mutex);
    // get pending msg list
    struct list_head msg_run;
    INIT_LIST_HEAD(&msg_run);
    pthread_mutex_lock(&obj->msg_lock);
    list_splice_init(&obj->msg_head, &msg_run);
    pthread_mutex_unlock(&obj->msg_lock);
    // run msg handler
    spe_msg_t* msg;
    spe_msg_t* tmp;
    list_for_each_entry_safe(msg, tmp, &msg_run, msg_node) {
      list_del_init(&msg->msg_node);
      if (obj->handler) obj->handler(obj, msg);
      free(msg);
    }
    // set object status
    pthread_mutex_lock(&worker_queue_mutex);
    if (list_empty(&obj->msg_head)) {
      obj->status = SPE_OBJECT_FREE;
      pthread_mutex_unlock(&worker_queue_mutex);
      continue;
    }
    list_add_tail(&obj->worker_node, &worker_queue);
    obj->status = SPE_OBJECT_QUEUE;
    pthread_mutex_unlock(&worker_queue_mutex);
  }
  return NULL;
}

/*
===================================================================================================
spe_worker_run
===================================================================================================
*/
void
spe_worker_run(void) {
  int num = spe_cpu_count(); 
  if (num > MAX_WORKER) num = MAX_WORKER; 
  for (int i=0; i<num; i++) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, worker_routine, NULL);
    pthread_attr_destroy(&attr);
  }
}
