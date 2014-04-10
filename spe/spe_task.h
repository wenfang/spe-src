#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

struct spe_task_s {
  spe_handler_t     handler;
  unsigned long     expire;
  struct rb_node    timer_node;
  struct list_head  task_node;
  unsigned          timeout:1;
};
typedef struct spe_task_s spe_task_t;

extern bool
spe_task_empty();

extern void
spe_task_init(spe_task_t* task);

extern void
spe_task_enqueue(spe_task_t* task);

extern void
spe_task_dequeue(spe_task_t* task);

extern void
spe_task_process();

#endif
