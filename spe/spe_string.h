#ifndef __SPE_STRING_H
#define __SPE_STRING_H

#include "spe_util.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct spe_string_s {
  char*     data;
  unsigned  len;        // used length, string length
  unsigned  _size;      // total size
  char*     _start;     // point to the real buffer
  char      _buffer[0]; // default data buffer
};
typedef struct spe_string_s spe_string_t;

struct spe_slist_s {
  spe_string_t**  data;   // spe_string list
  unsigned        len;    // used length
  unsigned        _size;  // count of spe_string
};
typedef struct spe_slist_s spe_slist_t;

extern bool 
spe_string_catb(spe_string_t* dst, const char* src, unsigned len);

extern bool 
spe_string_copyb(spe_string_t* dst, const char* src, unsigned len);

extern int  
spe_string_cmp(spe_string_t* str1, spe_string_t* str2);

extern int  
spe_string_consume(spe_string_t* str, unsigned len);

extern int  
spe_string_rconsume(spe_string_t* str, unsigned len);

extern int  
spe_string_search(spe_string_t* str, const char* key);

extern spe_slist_t* 
spe_string_split(spe_string_t* str, const char* key);

extern void 
spe_string_lstrim(spe_string_t* str);

extern void 
spe_string_rstrim(spe_string_t* str);

extern void
spe_string_tolower(spe_string_t* str);

extern void
spe_string_toupper(spe_string_t* str);

extern int
spe_string_read(int fd, spe_string_t* str, unsigned len);

extern int
spe_string_read_append(int fd, spe_string_t* str, unsigned len);

extern spe_string_t* 
spe_string_create(unsigned default_size);

extern void
spe_string_destroy(spe_string_t* str);

static inline bool 
spe_string_cat(spe_string_t* dst, spe_string_t* src) {
  ASSERT(dst && src);
  return spe_string_catb(dst, src->data, src->len);
}

static inline bool 
spe_string_cats(spe_string_t* dst, const char* src) {
  ASSERT(dst && src);
  return spe_string_catb(dst, src, strlen(src));
}

static inline bool 
spe_string_copy(spe_string_t* dst, spe_string_t* src) {
  ASSERT(dst && src);
  return spe_string_copyb(dst, src->data, src->len);
}

static inline bool 
spe_string_copys(spe_string_t* dst, const char* src) {
  ASSERT(dst && src);
  return spe_string_copyb(dst, src, strlen(src));
}

static inline void 
spe_string_clean(spe_string_t* str) {
  ASSERT(str);
  str->data     = str->_start;
  str->len      = 0;
  str->data[0]  = 0;
}

static inline void 
spe_string_strim(spe_string_t* str) {
  ASSERT(str);
  spe_string_lstrim(str);
  spe_string_rstrim(str);
}

extern bool 
spe_slist_appendb(spe_slist_t* slist, char* str, unsigned len);

static inline spe_slist_t*
spe_slist_create() {
  return calloc(1, sizeof(spe_slist_t));
}

static inline void
spe_slist_destroy(spe_slist_t* slist) {
  if (!slist) return;
  for (int i = 0; i < slist->_size; i++) {
    if (slist->data[i]) spe_string_destroy(slist->data[i]);
  }
  free(slist->data);
  free(slist);
}

static inline bool 
spe_slist_append(spe_slist_t* slist, spe_string_t* str) {
  ASSERT(slist && str);
  return spe_slist_appendb(slist, str->data, str->len);
}

static inline bool 
spe_slist_appends(spe_slist_t* slist, char* str) {
  ASSERT(slist && str);
  return spe_slist_appendb(slist, str, strlen(str));
}

static inline void 
spe_slist_clean(spe_slist_t* slist) {
  ASSERT(slist);
  slist->len = 0;
}

#endif
