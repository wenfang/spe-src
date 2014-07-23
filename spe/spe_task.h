#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

struct SpeTask_s {
  spe_handler_t     Handler;
  unsigned long     expire;
  struct rb_node    timerNode;
  struct list_head  taskNode;
  unsigned          status:7;
  unsigned          Timeout:1;
};
typedef struct SpeTask_s SpeTask_t;

extern unsigned gTaskNum;

extern void
SpeTaskInit(SpeTask_t* task);

extern bool 
SpeTaskEnqueue(SpeTask_t* task);

extern bool
SpeTaskDequeue(SpeTask_t* task);

extern bool
SpeTaskEnqueueTimer(SpeTask_t* task, unsigned long ms);

extern bool
SpeTaskDequeueTimer(SpeTask_t* task);

extern bool
SpeThreadTaskEnqueue(SpeTask_t* task);

extern bool
SpeThreadTaskDequeue(SpeTask_t* task);

extern void
speTaskProcess();

#endif
