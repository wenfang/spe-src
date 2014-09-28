#ifndef __SPE_OPT_H
#define __SPE_OPT_H

#include <stdbool.h>

extern int
SpeOptInt(char* section, char* key, int defaultValue);

extern const char*
SpeOptString(char* section, char* key, const char* defaultValue);

extern bool 
speOptCreate(const char* configFile);

extern void
speOptDestroy();

#endif
