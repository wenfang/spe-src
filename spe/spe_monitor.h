#ifndef __SPE_MONITOR_H
#define __SPE_MONITOR_H

#include <stdbool.h>

extern void
speMonitorStart();

extern bool
speMonitorInit(const char* addr, int port);

extern void
speMonitorDeinit();

#endif
