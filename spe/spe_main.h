#ifndef __SPE_MAIN_H
#define __SPE_MAIN_H

#include "spe_server.h"
#include <stdbool.h>

extern bool g_stop;

extern bool
spe_main_procs(int procs);

/*******************user define the following functions *******************/
extern bool
mod_init(void);

extern bool
mod_exit(void);

#endif
