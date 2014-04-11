#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

static LIST_HEAD(task_head);
static pthread_mutex_t  task_lock = PTHREAD_MUTEX_INITIALIZER;

/*
===================================================================================================
spe_task_empty
===================================================================================================
*/
bool
spe_task_empty() {
  return list_empty(&task_head);
}

/*
===================================================================================================
spe_task_init
===================================================================================================
*/
void
spe_task_init(spe_task_t* task) {
  ASSERT(task);
  task->handler = SPE_HANDLER_NULL;
  task->expire  = 0;
  rb_init_node(&task->timer_node);
  INIT_LIST_HEAD(&task->task_node);
  task->inqueue = 0;
  task->timeout = 0;
}

/*
===================================================================================================
spe_task_enqueue
===================================================================================================
*/
void
spe_task_enqueue(spe_task_t* task) {
  ASSERT(task);
  // double check
  if (task->inqueue) return;
  pthread_mutex_lock(&task_lock);
  if (task->inqueue) {
    pthread_mutex_lock(&task_lock);
    return;
  }
  list_add_tail(&task->task_node, &task_head);
  task->inqueue = 1;
  pthread_mutex_unlock(&task_lock);
  spe_epoll_wakeup();
}

/*
===================================================================================================
spe_task_dequeue
===================================================================================================
*/
void
spe_task_dequeue(spe_task_t* task) {
  ASSERT(task);
  // double check
  if (!task->inqueue) return;
  pthread_mutex_lock(&task_lock);
  if (!task->inqueue) {
    pthread_mutex_unlock(&task_lock);
    return;
  }
  list_del_init(&task->task_node);
  task->inqueue = 0;
  pthread_mutex_unlock(&task_lock);
}

/*
===================================================================================================
spe_task_process
===================================================================================================
*/
void
spe_task_process() {
  // run task
  while (!list_empty(&task_head)) {
    pthread_mutex_lock(&task_lock);
    spe_task_t* task = list_first_entry(&task_head, spe_task_t, task_node);
    if (!task) {
      pthread_mutex_unlock(&task_lock);
      break;
    }
    list_del_init(&task->task_node);
    task->inqueue = 0;
    pthread_mutex_unlock(&task_lock);
    SPE_HANDLER_CALL(task->handler);
  }
}
