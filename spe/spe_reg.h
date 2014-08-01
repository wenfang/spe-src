#ifndef __SPEREG_H
#define __SPEREG_H

#include "spe_string.h"
#include <regex.h>

struct SpeReg_s {
  regex_t       preg;
  spe_slist_t*  Result;
};
typedef struct SpeReg_s SpeReg_t;

extern bool   
SpeRegSearch(SpeReg_t* reg, char* string);

extern SpeReg_t*
SpeRegCreate(const char* regex);

extern void
SpeRegDestroy(SpeReg_t* reg);

#endif
