#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_shm.h"
#include "spe_task.h"
#include "spe_epoll.h"

typedef void (*spe_server_Handler)(unsigned);

extern bool
speServerUseAcceptMutex();

extern void
speServerStart();

extern void
speServerBeforeLoop();

extern void
speServerAfterLoop();

// user call spe_server_init and spe_server_deinit
extern bool
SpeServerInit(const char* addr, int port, spe_server_Handler handler);

extern void
SpeServerDeinit();

#endif
