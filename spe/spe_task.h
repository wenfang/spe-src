#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

struct spe_task_s {
  spe_handler_t     handler;
  unsigned long     _expire;
  struct rb_node    _timer_node;
  struct list_head  _task_node;
  unsigned          _intimer:1;   // task in timer queue?
  unsigned          _intask:1;    // task in running queue?
  unsigned          timeout:1;
};
typedef struct spe_task_s spe_task_t;

extern unsigned g_task_num;

extern void
spe_task_init(spe_task_t* task);

extern void
spe_task_enqueue(spe_task_t* task);

extern void
spe_task_dequeue(spe_task_t* task);

extern void
spe_task_enqueue_timer(spe_task_t* task, unsigned long ms);

extern void
spe_task_dequeue_timer(spe_task_t* task);

extern void
spe_task_process();

extern void
spe_timer_process();


#endif
