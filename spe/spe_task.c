#include "spe_task.h"
#include "spe_list.h"
#include "spe_handler.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

static LIST_HEAD(task_head);
static pthread_mutex_t  task_lock = PTHREAD_MUTEX_INITIALIZER;

struct spe_task_s {
  int               key;
  spe_handler_t     handler;
  struct list_head  task_node;
};
typedef struct spe_task_s spe_task_t;

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
spe_task_add
===================================================================================================
*/
void
spe_task_add(int key, spe_handler_t handler) {
  spe_task_t* task = calloc(1, sizeof(spe_task_t));
  if (unlikely(!task)) {
    SPE_LOG_ERR("spe_task calloc error");
    return;
  }
  task->key     = key;
  task->handler = handler;
  INIT_LIST_HEAD(&task->task_node);
  pthread_mutex_lock(&task_lock);
  list_add_tail(&task->task_node, &task_head);
  pthread_mutex_unlock(&task_lock);
  spe_epoll_wakeup();
}

/*
===================================================================================================
spe_task_del
===================================================================================================
*/
void
spe_task_del(int key) {
  spe_task_t* task;
  spe_task_t* tmp;
  pthread_mutex_lock(&task_lock);
  list_for_each_entry_safe(task, tmp, &task_head, task_node) {
    if (task->key == key) {
      list_del_init(&task->task_node);
      free(task);
    }
  }
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
    pthread_mutex_unlock(&task_lock);
    SPE_HANDLER_CALL(task->handler);
    free(task);
  }
}
