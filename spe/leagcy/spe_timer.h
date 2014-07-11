#ifndef __SPE_TIMER_H
#define __SPE_TIMER_H

#include "spe_task.h"

extern void
spe_timer_enable(spe_task_t* task, unsigned long ms);

extern void
spe_timer_disable(spe_task_t* task);

extern void
spe_timer_process();

#endif
