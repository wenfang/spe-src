#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include <stdbool.h>

typedef void (*speServerHandler)(unsigned);

extern bool
speServerUseAcceptMutex();

extern void
speServerStart();

extern void
speServerBeforeLoop();

extern void
speServerAfterLoop();

extern bool
SpeServerInit(const char* addr, int port, speServerHandler handler);

extern void
SpeServerDeinit();

#endif
