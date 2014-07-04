#ifndef __SPE_MAIN_H
#define __SPE_MAIN_H

#include "spe_task.h"
#include <stdbool.h>

extern bool modStop;
extern spe_task_t modTask;

extern bool
mod_init(void);

extern bool
mod_exit(void);

extern void
mod_before_loop(void);

extern void
mod_after_loop(void);

#endif
