#ifndef __SPE_MONITOR_H
#define __SPE_MONITOR_H

#include <stdbool.h>

extern bool
SpeMonitorInit(const char* addr, int port);

extern void
SpeMonitorDeInit();

#endif
