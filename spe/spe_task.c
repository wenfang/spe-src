#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

#define SPE_TASK_FREE   0
#define SPE_TASK_TIMER  1
#define SPE_TASK_QUEUE  2
#define SPE_TASK_RUN    4

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
  task->Handler = SPE_HANDLER_NULL;
  task->expire  = 0;
  rb_init_node(&task->timerNode);
  INIT_LIST_HEAD(&task->taskNode);
  task->status  = SPE_TASK_FREE;
  task->Timeout = 0;
}

/*
===================================================================================================
SpeTaskEnqueue
===================================================================================================
*/
bool
SpeTaskEnqueue(SpeTask_t* task) {
  ASSERT(task);
  // double check
  if (task->status == SPE_TASK_QUEUE || task->status == SPE_TASK_RUN) return false;
  pthread_mutex_lock(&task_lock);
  if (task->status == SPE_TASK_QUEUE || task->status == SPE_TASK_RUN) {
    pthread_mutex_lock(&task_lock);
    return false;
  }
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timerNode, &timer_head);
    rb_init_node(&task->timerNode);
  }
  list_add_tail(&task->taskNode, &task_head);
  task->status = SPE_TASK_QUEUE;
  gTaskNum++;
  pthread_mutex_unlock(&task_lock);
  speEpollWakeup();
  return true;
}

/*
===================================================================================================
SpeTaskDequeue
===================================================================================================
*/
bool
SpeTaskDequeue(SpeTask_t* task) {
  ASSERT(task);
  // double check
  if (task->status != SPE_TASK_QUEUE) return false;
  pthread_mutex_lock(&task_lock);
  if (task->status != SPE_TASK_QUEUE) {
    pthread_mutex_unlock(&task_lock);
    return false;
  }
  list_del_init(&task->taskNode);
  task->status = SPE_TASK_FREE;
  gTaskNum--;
  pthread_mutex_unlock(&task_lock);
  return true;
}

/*
===================================================================================================
SpeTaskEnqueue_timer
===================================================================================================
*/
bool
SpeTaskEnqueueTimer(SpeTask_t* task, unsigned long ms) {
  ASSERT(task);
  if (task->status == SPE_TASK_QUEUE || task->status == SPE_TASK_RUN) return false;
  // insert into timer list
  pthread_mutex_lock(&task_lock);
  if (task->status == SPE_TASK_QUEUE || task->status == SPE_TASK_RUN) {
    pthread_mutex_unlock(&task_lock);
    return false;
  }
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timerNode, &timer_head);
    rb_init_node(&task->timerNode);
  }
  task->expire = spe_current_time() + ms;
  task->Timeout = 0;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    SpeTask_t* curr = rb_entry(*new, SpeTask_t, timerNode);
    parent = *new;
    if (task->expire < curr->expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->timerNode, parent, new);
  rb_insert_color(&task->timerNode, &timer_head);
  task->status = SPE_TASK_TIMER;
  pthread_mutex_unlock(&task_lock);
  return true;
}

/*
===================================================================================================
SpeTaskDequeueTimer
===================================================================================================
*/
bool
SpeTaskDequeueTimer(SpeTask_t* task) {
  ASSERT(task);
  if (task->status != SPE_TASK_TIMER) return false;
  pthread_mutex_lock(&task_lock);
  if (task->status != SPE_TASK_TIMER) {
    pthread_mutex_lock(&task_lock);
    return false;
  }
  rb_erase(&task->timerNode, &timer_head);
  task->status = SPE_TASK_FREE;
  rb_init_node(&task->timerNode);
  pthread_mutex_unlock(&task_lock);
  return true;
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
      SpeTask_t* task = rb_entry(first, SpeTask_t, timerNode);
      if (task->expire > curr_time) break;
      ASSERT(task->status == SPE_TASK_TIMER);
      rb_erase(&task->timerNode, &timer_head);
      rb_init_node(&task->timerNode);
      task->Timeout = 1;
      // add to task queue
      list_add_tail(&task->taskNode, &task_head);
      task->status  = SPE_TASK_QUEUE;
      gTaskNum++;
      first = rb_first(&timer_head);
    } 
    pthread_mutex_unlock(&task_lock);
  }
  // run task
  while (gTaskNum) {
    pthread_mutex_lock(&task_lock);
    SpeTask_t* task = list_first_entry(&task_head, SpeTask_t, taskNode);
    if (!task) {
      pthread_mutex_unlock(&task_lock);
      break;
    }
    list_del_init(&task->taskNode);
    ASSERT(task->status == SPE_TASK_QUEUE);
    task->status = SPE_TASK_RUN;
    gTaskNum--;
    pthread_mutex_unlock(&task_lock);
    SPE_HANDLER_CALL(task->Handler);
    // set task stauts to free
    pthread_mutex_lock(&task_lock);
    task->status = SPE_TASK_FREE;
    pthread_mutex_unlock(&task_lock);
  }
}

__attribute__((constructor))
static void
task_init(void) {
  timer_head = RB_ROOT;
}
