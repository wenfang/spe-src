#ifndef __SPE_TIMER_H
#define __SPE_TIMER_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"

struct spe_timer_s {
  unsigned long     expire;
  spe_handler_t     handler;
  struct rb_node    node;
  unsigned          timeout:1;
};
typedef struct spe_timer_s spe_timer_t;

extern spe_timer_t*
spe_timer_create();

extern void
spe_timer_destroy(spe_timer_t* timer);

extern void
spe_timer_enable(spe_timer_t* timer, unsigned long ms, spe_handler_t handler);

extern void
spe_timer_disable(spe_timer_t* timer);

extern void
spe_timer_process();

#endif
