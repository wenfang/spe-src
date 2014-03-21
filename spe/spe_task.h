#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_handler.h"
#include <stdbool.h>

extern bool
spe_task_empty();

extern void
spe_task_add(int key, spe_handler_t handler);

extern void
spe_task_del(int key);

extern void
spe_task_process();

#endif
