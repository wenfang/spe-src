#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

unsigned gTaskNum;

static LIST_HEAD(task_head);
static struct rb_root   timer_head;
static pthread_mutex_t  task_lock = PTHREAD_MUTEX_INITIALIZER;

/*
===================================================================================================
SpeTaskInit
===================================================================================================
*/
void
SpeTaskInit(SpeTask_t* task) {
  ASSERT(task);
  task->handler   = SPE_HANDLER_NULL;
  task->_expire   = 0;
  rb_init_node(&task->_timer_node);
  INIT_LIST_HEAD(&task->_task_node);
  task->_intimer  = 0;
  task->_intask   = 0;
  task->timeout   = 0;
}

/*
===================================================================================================
SpeTaskEnqueue
===================================================================================================
*/
void
SpeTaskEnqueue(SpeTask_t* task) {
  ASSERT(task);
  // double check
  if (task->_intask) return;
  pthread_mutex_lock(&task_lock);
  if (task->_intask) {
    pthread_mutex_lock(&task_lock);
    return;
  }
  if (task->_intimer) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
    task->_intimer = 0;
  }
  list_add_tail(&task->_task_node, &task_head);
  task->_intask = 1;
  gTaskNum++;
  pthread_mutex_unlock(&task_lock);
  spe_epoll_wakeup();
}

/*
===================================================================================================
SpeTaskDequeue
===================================================================================================
*/
void
SpeTaskDequeue(SpeTask_t* task) {
  ASSERT(task);
  // double check
  if (!task->_intask) return;
  pthread_mutex_lock(&task_lock);
  if (!task->_intask) {
    pthread_mutex_unlock(&task_lock);
    return;
  }
  list_del_init(&task->_task_node);
  task->_intask = 0;
  gTaskNum--;
  pthread_mutex_unlock(&task_lock);
}

/*
===================================================================================================
SpeTaskEnqueue_timer
===================================================================================================
*/
void
SpeTaskEnqueueTimer(SpeTask_t* task, unsigned long ms) {
  ASSERT(task);
  if (task->_intask) return;
  // insert into timer list
  pthread_mutex_lock(&task_lock);
  if (task->_intask) {
    pthread_mutex_unlock(&task_lock);
    return;
  }
  if (task->_intimer) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
    task->_intimer = 0;
  }
  task->_expire = spe_current_time() + ms;
  task->timeout = 0;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    SpeTask_t* curr = rb_entry(*new, SpeTask_t, _timer_node);
    parent = *new;
    if (task->_expire < curr->_expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->_timer_node, parent, new);
  rb_insert_color(&task->_timer_node, &timer_head);
  task->_intimer = 1;
  pthread_mutex_unlock(&task_lock);
}

/*
===================================================================================================
SpeTaskDequeueTimer
===================================================================================================
*/
void
SpeTaskDequeueTimer(SpeTask_t* task) {
  ASSERT(task);
  if (!task->_intimer) return;
  pthread_mutex_lock(&task_lock);
  if (!task->_intimer) {
    pthread_mutex_lock(&task_lock);
    return;
  }
  rb_erase(&task->_timer_node, &timer_head);
  task->_intimer = 0;
  rb_init_node(&task->_timer_node);
  pthread_mutex_unlock(&task_lock);
}

/*
===================================================================================================
spe_task_process
===================================================================================================
*/
void
speTaskProcess() {
  // check timer queue
  if (!RB_EMPTY_ROOT(&timer_head)) {
    unsigned long curr_time = spe_current_time();
    // check timer list
    pthread_mutex_lock(&task_lock);
    struct rb_node* first = rb_first(&timer_head);
    while (first) {
      SpeTask_t* task = rb_entry(first, SpeTask_t, _timer_node);
      if (task->_expire > curr_time) break;
      rb_erase(&task->_timer_node, &timer_head);
      rb_init_node(&task->_timer_node);
      task->_intimer = 0;
      task->timeout = 1;
      // add to task queue
      list_add_tail(&task->_task_node, &task_head);
      task->_intask = 1;
      gTaskNum++;
      first = rb_first(&timer_head);
    } 
    pthread_mutex_unlock(&task_lock);
  }
  // run task
  while (gTaskNum) {
    pthread_mutex_lock(&task_lock);
    SpeTask_t* task = list_first_entry(&task_head, SpeTask_t, _task_node);
    if (!task) {
      pthread_mutex_unlock(&task_lock);
      break;
    }
    list_del_init(&task->_task_node);
    task->_intask = 0;
    gTaskNum--;
    pthread_mutex_unlock(&task_lock);
    SPE_HANDLER_CALL(task->handler);
  }
}

__attribute__((constructor))
static void
task_init(void) {
  timer_head = RB_ROOT;
}
