#ifndef __SPE_STRING_H
#define __SPE_STRING_H

#include "spe_util.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct SpeString_s {
  char*     data;
  unsigned  len;        // used length, string length
  unsigned  _size;      // total size
  char*     _start;     // point to the real buffer
  char      _buffer[0]; // default data buffer
};
typedef struct SpeString_s SpeString_t;

struct SpeSlist_s {
  SpeString_t** data;   // spe_string list
  unsigned      len;    // used length
  unsigned      _size;  // count of spe_string
};
typedef struct SpeSlist_s SpeSlist_t;

extern bool 
SpeStringCatb(SpeString_t* dst, const char* src, unsigned len);

extern bool 
SpeStringCopyb(SpeString_t* dst, const char* src, unsigned len);

extern int  
SpeStringCmp(SpeString_t* str1, SpeString_t* str2);

extern int  
SpeStringConsume(SpeString_t* str, unsigned len);

extern int  
SpeStringRconsume(SpeString_t* str, unsigned len);

extern int  
SpeStringSearch(SpeString_t* str, const char* key);

extern SpeSlist_t* 
SpeStringSplit(SpeString_t* str, const char* key);

extern void 
SpeStringLstrim(SpeString_t* str);

extern void 
SpeStringRstrim(SpeString_t* str);

extern void
SpeStringToLower(SpeString_t* str);

extern void
SpeStringToUpper(SpeString_t* str);

extern int
SpeStringRead(int fd, SpeString_t* str, unsigned len);

extern int
SpeStringReadAppend(int fd, SpeString_t* str, unsigned len);

extern SpeString_t* 
SpeStringCreate(unsigned defaultSize);

extern void
SpeStringDestroy(SpeString_t* str);

static inline bool 
SpeStringCat(SpeString_t* dst, SpeString_t* src) {
  ASSERT(dst && src);
  return SpeStringCatb(dst, src->data, src->len);
}

static inline bool 
SpeStringCats(SpeString_t* dst, const char* src) {
  ASSERT(dst && src);
  return SpeStringCatb(dst, src, strlen(src));
}

static inline bool 
SpeStringCopy(SpeString_t* dst, SpeString_t* src) {
  ASSERT(dst && src);
  return SpeStringCopyb(dst, src->data, src->len);
}

static inline bool 
SpeStringCopys(SpeString_t* dst, const char* src) {
  ASSERT(dst && src);
  return SpeStringCopyb(dst, src, strlen(src));
}

static inline void 
SpeStringClean(SpeString_t* str) {
  ASSERT(str);
  str->data     = str->_start;
  str->len      = 0;
  str->data[0]  = 0;
}

static inline void 
SpeStringStrim(SpeString_t* str) {
  ASSERT(str);
  SpeStringLstrim(str);
  SpeStringRstrim(str);
}

extern bool 
spe_slist_appendb(SpeSlist_t* slist, char* str, unsigned len);

static inline SpeSlist_t*
spe_slist_create() {
  return calloc(1, sizeof(SpeSlist_t));
}

static inline void
spe_slist_destroy(SpeSlist_t* slist) {
  if (!slist) return;
  for (int i = 0; i < slist->_size; i++) {
    if (slist->data[i]) SpeStringDestroy(slist->data[i]);
  }
  free(slist->data);
  free(slist);
}

static inline bool 
spe_slist_append(SpeSlist_t* slist, SpeString_t* str) {
  ASSERT(slist && str);
  return spe_slist_appendb(slist, str->data, str->len);
}

static inline bool 
spe_slist_appends(SpeSlist_t* slist, char* str) {
  ASSERT(slist && str);
  return spe_slist_appendb(slist, str, strlen(str));
}

static inline void 
spe_slist_clean(SpeSlist_t* slist) {
  ASSERT(slist);
  slist->len = 0;
}

#endif
