#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

struct SpeTask_s {
  spe_handler_t     handler;
  unsigned long     _expire;
  struct rb_node    _timer_node;
  struct list_head  _task_node;
  unsigned          _intimer:1;   // task in timer queue?
  unsigned          _intask:1;    // task in running queue?
  unsigned          timeout:1;
};
typedef struct SpeTask_s SpeTask_t;

extern unsigned gTaskNum;

extern void
SpeTaskInit(SpeTask_t* task);

extern void
SpeTaskEnqueue(SpeTask_t* task);

extern void
SpeTaskDequeue(SpeTask_t* task);

extern void
SpeTaskEnqueueTimer(SpeTask_t* task, unsigned long ms);

extern void
SpeTaskDequeueTimer(SpeTask_t* task);

extern void
speTaskProcess();

#endif
