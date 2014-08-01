#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_conn.h"
#include <stdbool.h>

typedef void (*SpeServerHandler)(spe_conn_t*);

extern bool
serverUseAcceptMutex();

extern void
serverEnable();

extern void
serverBeforeLoop();

extern void
serverAfterLoop();

//*************************user use this********************************
extern bool
SpeServerInit(const char* addr, int port, SpeServerHandler handler);

extern void
SpeServerDeinit();

#endif
