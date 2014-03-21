#ifndef __SPE_OPT_H
#define __SPE_OPT_H

#include <stdbool.h>

extern int
spe_opt_int(char* sec, char* key, int defval);

extern const char*
spe_opt_string(char* sec, char* key, const char* defval);

extern bool 
spe_opt_create(const char* fname);

extern void
spe_opt_destroy();

#endif
