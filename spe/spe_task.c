#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

unsigned g_task_num;

static LIST_HEAD(task_head);
static pthread_mutex_t  task_lock = PTHREAD_MUTEX_INITIALIZER;

static struct rb_root   timer_head;
static pthread_mutex_t  timer_lock = PTHREAD_MUTEX_INITIALIZER;

/*
===================================================================================================
spe_task_init
===================================================================================================
*/
void
spe_task_init(spe_task_t* task) {
  ASSERT(task);
  task->handler = SPE_HANDLER_NULL;
  task->_expire = 0;
  rb_init_node(&task->_timer_node);
  INIT_LIST_HEAD(&task->_task_node);
  task->_intimer  = 0;
  task->_intask   = 0;
  task->timeout   = 0;
}

/*
===================================================================================================
spe_task_enqueue
===================================================================================================
*/
void
spe_task_enqueue(spe_task_t* task) {
  ASSERT(task && !task->_intimer);
  // double check
  if (task->_intask) return;
  pthread_mutex_lock(&task_lock);
  if (task->_intask) {
    pthread_mutex_lock(&task_lock);
    return;
  }
  list_add_tail(&task->_task_node, &task_head);
  task->_intask = 1;
  g_task_num++;
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
  if (!task->_intask) return;
  pthread_mutex_lock(&task_lock);
  if (!task->_intask) {
    pthread_mutex_unlock(&task_lock);
    return;
  }
  list_del_init(&task->_task_node);
  task->_intask = 0;
  g_task_num--;
  pthread_mutex_unlock(&task_lock);
}

/*
===================================================================================================
spe_task_enqueue_timer
===================================================================================================
*/
void
spe_task_enqueue_timer(spe_task_t* task, unsigned long ms) {
  ASSERT(task && !task->_intask);
  // disable timer if timer is valid
  spe_task_dequeue_timer(task);
  task->_expire = spe_current_time() + ms;
  task->timeout = 0;
  // insert into timer list
  pthread_mutex_lock(&timer_lock);
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    spe_task_t* curr = rb_entry(*new, spe_task_t, _timer_node);
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
  pthread_mutex_unlock(&timer_lock);
}

/*
===================================================================================================
spe_task_dequeue_timer
===================================================================================================
*/
void
spe_task_dequeue_timer(spe_task_t* task) {
  ASSERT(task);
  if (!task->_intimer) return;
  pthread_mutex_lock(&timer_lock);
  if (!task->_intimer) {
    pthread_mutex_lock(&timer_lock);
    return;
  }
  rb_erase(&task->_timer_node, &timer_head);
  task->_intimer = 0;
  pthread_mutex_unlock(&timer_lock);
  rb_init_node(&task->_timer_node);
}

/*
===================================================================================================
spe_timer_process
===================================================================================================
*/
void
spe_timer_process() {
  if (RB_EMPTY_ROOT(&timer_head)) return;
  unsigned long curr_time = spe_current_time();
  // check timer list
  pthread_mutex_lock(&timer_lock);
  struct rb_node* first = rb_first(&timer_head);
  while (first) {
    spe_task_t* task = rb_entry(first, spe_task_t, _timer_node);
    if (task->_expire > curr_time) break;
    rb_erase(&task->_timer_node, &timer_head);
    task->_intimer = 0;
    pthread_mutex_unlock(&timer_lock);
    rb_init_node(&task->_timer_node);
    // run timer
    task->timeout = 1;
    spe_task_enqueue(task);
    // try next
    pthread_mutex_lock(&timer_lock);
    first = rb_first(&timer_head);
  } 
  pthread_mutex_unlock(&timer_lock);
}

/*
===================================================================================================
spe_task_process
===================================================================================================
*/
void
spe_task_process() {
  // run task
  while (g_task_num) {
    pthread_mutex_lock(&task_lock);
    spe_task_t* task = list_first_entry(&task_head, spe_task_t, _task_node);
    if (!task) {
      pthread_mutex_unlock(&task_lock);
      break;
    }
    list_del_init(&task->_task_node);
    task->_intask = 0;
    g_task_num--;
    pthread_mutex_unlock(&task_lock);
    SPE_HANDLER_CALL(task->handler);
  }
}

__attribute__((constructor))
static void
task_init(void) {
  timer_head = RB_ROOT;
}
