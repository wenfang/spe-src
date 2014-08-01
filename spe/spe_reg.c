#include "spe_reg.h"
#include "spe_log.h"
#include <stdlib.h>
#include <string.h>

#define MAXLEN  16

/*
===================================================================================================
SpeRegSearch
===================================================================================================
*/
bool 
SpeRegSearch(SpeReg_t* reg, char* string) {
  ASSERT(reg && string);
  // call regexec to search string
  regmatch_t pm[MAXLEN];
  if (regexec(&reg->preg, string, MAXLEN, pm, 0)) return false;
  spe_slist_clean(reg->Result);
  // copy data 
  for (int i=0; i < MAXLEN && pm[i].rm_so != -1; i++) {
    spe_slist_appendb(reg->Result, string + pm[i].rm_so, pm[i].rm_eo - pm[i].rm_so);
  }
  return true;
}

/*
===================================================================================================
SpeRegCreate
===================================================================================================
*/
SpeReg_t*
SpeRegCreate(const char* regex) {
  SpeReg_t* reg = calloc(1, sizeof(SpeReg_t));
  if (!reg) {
    SPE_LOG_ERR("SpeReg_t calloc error");
    return NULL;
  }
  reg->Result = spe_slist_create();
  if (!reg->Result) {
    SPE_LOG_ERR("spe_slist_create error");
    free(reg);
    return NULL;
  }
  // init reg
  if (regcomp(&reg->preg, regex, REG_EXTENDED)) {
    SPE_LOG_ERR("regcom error");
    spe_slist_destroy(reg->Result);
    free(reg);
    return NULL;
  }
  return reg;
}

/*
===================================================================================================
SpeRegDestroy
===================================================================================================
*/
void
SpeRegDestroy(SpeReg_t* reg) {
  if (!reg) return;
  spe_slist_destroy(reg->Result);
  regfree(&reg->preg);
  free(reg);
}
