#ifndef __SPE_HTTP_SERVER_H
#define __SPE_HTTP_SERVER_H

#include <stdbool.h>

extern bool
SpeHttpServerInit(const char* addr, int port);

extern void
SpeHttpServerDeinit();

#endif
