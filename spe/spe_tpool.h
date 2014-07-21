#ifndef __SPE_TPOOL_H
#define __SPE_TPOOL_H

#include "spe_handler.h"
#include <stdbool.h>

extern bool
SpeTpoolDo(spe_handler_t handler);

extern bool
SpeTpoolInit();

extern void
SpeTpoolDeinit();

#endif
