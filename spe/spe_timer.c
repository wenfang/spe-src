#include "spe_timer.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

static struct rb_root   timer_list;
static pthread_mutex_t  timer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
===================================================================================================
spe_timer_create
===================================================================================================
*/
spe_timer_t*
spe_timer_create() {
  spe_timer_t* timer = calloc(1, sizeof(spe_timer_t));
  if (!timer) {
    SPE_LOG_ERR("calloc error");
    return NULL;
  }
  RB_CLEAR_NODE(&timer->node);
  return timer;
}

/*
===================================================================================================
spe_timer_destroy
===================================================================================================
*/
void
spe_timer_destroy(spe_timer_t* timer) {
  ASSERT(timer);
  spe_timer_disable(timer);
  free(timer);
}

/*
===================================================================================================
spe_timer_enable
===================================================================================================
*/
void
spe_timer_enable(spe_timer_t* timer, unsigned long ms, spe_handler_t handler) {
  ASSERT(timer);
  // disable timer if timer is valid
  spe_timer_disable(timer);
  timer->expire   = spe_current_time() + ms;
  timer->handler  = handler;
  timer->timeout  = 0;
  // insert into timer list
  pthread_mutex_lock(&timer_list_mutex);
  struct rb_node **new = &timer_list.rb_node, *parent = NULL;
  while (*new) {
    spe_timer_t* curr = rb_entry(*new, spe_timer_t, node);
    parent = *new;
    if (timer->expire < curr->expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&timer->node, parent, new);
  rb_insert_color(&timer->node, &timer_list);
  pthread_mutex_unlock(&timer_list_mutex);
}

/*
===================================================================================================
spe_timer_disable
===================================================================================================
*/
void
spe_timer_disable(spe_timer_t* timer) {
  ASSERT(timer);
  if (RB_EMPTY_NODE(&timer->node)) return;
  pthread_mutex_lock(&timer_list_mutex);
  rb_erase(&timer->node, &timer_list);
  pthread_mutex_unlock(&timer_list_mutex);
  rb_init_node(&timer->node);
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
    spe_timer_t* timer = rb_entry(first, spe_timer_t, node);
    if (timer->expire > curr_time) break;
    rb_erase(&timer->node, &timer_list);
    pthread_mutex_unlock(&timer_list_mutex);
    rb_init_node(&timer->node);
    // run timer
    timer->timeout = 1;
    SPE_HANDLER_CALL(timer->handler);
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
