#include "spe_timer.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

static struct rb_root   timer_list;
static pthread_mutex_t  timer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
===================================================================================================
spe_timer_enable
===================================================================================================
*/
void
spe_timer_enable(spe_task_t* task, unsigned long ms) {
  ASSERT(task);
  // disable timer if timer is valid
  spe_timer_disable(task);
  task->expire  = spe_current_time() + ms;
  task->timeout = 0;
  // insert into timer list
  pthread_mutex_lock(&timer_list_mutex);
  struct rb_node **new = &timer_list.rb_node, *parent = NULL;
  while (*new) {
    spe_task_t* curr = rb_entry(*new, spe_task_t, timer_node);
    parent = *new;
    if (task->expire < curr->expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->timer_node, parent, new);
  rb_insert_color(&task->timer_node, &timer_list);
  pthread_mutex_unlock(&timer_list_mutex);
}

/*
===================================================================================================
spe_timer_disable
===================================================================================================
*/
void
spe_timer_disable(spe_task_t* task) {
  ASSERT(task);
  if (RB_EMPTY_NODE(&task->timer_node)) return;
  pthread_mutex_lock(&timer_list_mutex);
  rb_erase(&task->timer_node, &timer_list);
  pthread_mutex_unlock(&timer_list_mutex);
  rb_init_node(&task->timer_node);
}

/*
===================================================================================================
spe_timer_process
===================================================================================================
*/
void
spe_timer_process() {
  if (RB_EMPTY_ROOT(&timer_list)) return;
  unsigned long curr_time = spe_current_time();
  // check timer list
  pthread_mutex_lock(&timer_list_mutex);
  struct rb_node* first = rb_first(&timer_list);
  while (first) {
    spe_task_t* task = rb_entry(first, spe_task_t, timer_node);
    if (task->expire > curr_time) break;
    rb_erase(&task->timer_node, &timer_list);
    pthread_mutex_unlock(&timer_list_mutex);
    rb_init_node(&task->timer_node);
    // run timer
    task->timeout = 1;
    spe_task_enqueue(task);
    // try next
    pthread_mutex_lock(&timer_list_mutex);
    first = rb_first(&timer_list);
  } 
  pthread_mutex_unlock(&timer_list_mutex);
}

__attribute__((constructor))
static void
timer_init(void) {
  timer_list = RB_ROOT;
}
