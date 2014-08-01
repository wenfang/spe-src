#ifndef __SPE_MONITOR_H
#define __SPE_MONITOR_H

#include <stdbool.h>

extern void
monitorEnable();

extern bool
SpeMonitorInit(const char* addr, int port);

extern void
SpeMonitorDeinit();

#endif
