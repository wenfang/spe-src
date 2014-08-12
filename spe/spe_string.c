#include "spe_string.h"
#include "spe_log.h"

/*
===================================================================================================
SpeStringready
  alloc enough size for spe_string
===================================================================================================
*/
static bool 
SpeStringready(SpeString_t* str, unsigned need_size) {
  // 1. have enough size, return true
  if (need_size <= str->_size - (str->data - str->_start)) return true;
  // 2. no need to realloc
  if (need_size <= str->_size) {
    memmove(str->_start, str->data, str->len);
    str->data           = str->_start;
    str->data[str->len] = 0;
    return true;
  }
  // 3. need to realloc, get new size to alloc
  unsigned new_size = 30 + need_size + (need_size >> 3);
  char* new_data = malloc(new_size);
  if (!new_data) {
    SPE_LOG_ERR("malloc error");
    return false;
  }
  memcpy(new_data, str->data, str->len); 
  // free old buffer
  if (str->_start != str->_buffer) free(str->_start);
  str->_size          = new_size; 
  str->_start         = new_data;
  str->data           = str->_start;
  str->data[str->len] = 0;
  return true;
}

/*
===================================================================================================
SpeStringreadyplus
  call SpeStringready
===================================================================================================
*/
static inline bool 
SpeStringreadyplus(SpeString_t* str, unsigned need_size_plus) {
  return SpeStringready(str, str->len + need_size_plus);
}

/*
===================================================================================================
SpeStringCatb
  cat binary string to spe_string
===================================================================================================
*/
bool 
SpeStringCatb(SpeString_t* dst, const char* src, unsigned len) {
  ASSERT(dst && src);
  if (!SpeStringreadyplus(dst, len+1)) {
    SPE_LOG_ERR("SpeStringreadyplus error");
    return false;
  }
  memcpy(dst->data + dst->len, src, len);            
  dst->len            += len;
  dst->data[dst->len] = 0;                
  return true;
}

/*
===================================================================================================
SpeStringCopyb
  copy binary string to spe_string
===================================================================================================
*/
bool 
SpeStringCopyb(SpeString_t* dst, const char* src, unsigned len) {
  ASSERT(dst && src);
  if (!SpeStringready(dst, len+1)) {
    SPE_LOG_ERR("SpeStringready error");
    return false;
  }
  memcpy(dst->_start, src, len);
  dst->len            = len;          
  dst->data           = dst->_start;
  dst->data[dst->len] = 0;
  return true;
}

/*
===================================================================================================
SpeStringConsume
  consume len string from left
===================================================================================================
*/
int 
SpeStringConsume(SpeString_t* str, unsigned len) {
  ASSERT(str);
  if (len >= str->len) {
    len = str->len;
    SpeStringClean(str);
    return len; 
  }
  str->data           += len;
  str->len            -= len;
  str->data[str->len] = 0;
  return len;
}

/*
===================================================================================================
SpeStringrconsume
  consume str from right
===================================================================================================
*/
int 
SpeStringrconsume(SpeString_t* str, unsigned len) {
  ASSERT(str);
  if (len >= str->len) {
    len = str->len;
    SpeStringClean(str);
    return len;
  }
  str->len            -= len;
  str->data[str->len] = 0;
  return len;
}

/*
===================================================================================================
SpeString_search
    search string in x
    return startpositon in x
            -1 for no found or error
===================================================================================================
*/
int 
SpeString_search(SpeString_t* str, const char* key) {
  ASSERT(str && key);
  char* point = strstr(str->data, key);
  if (!point) return -1;
  return point - str->data;
}

/*
===================================================================================================
SpeStringlstrim
  strim string from left
===================================================================================================
*/
void 
SpeStringlstrim(SpeString_t* str) {
  ASSERT(str);
  int pos;
  for (pos = 0; pos < str->len; pos++) {
    if (str->data[pos] == '\t' || str->data[pos] == ' ' ||
        str->data[pos] == '\r' || str->data[pos] == '\n') continue;
    break;
  }
  if (pos) SpeStringConsume(str, pos);
}

/*
===================================================================================================
SpeStringrstrim
  strim string from right
===================================================================================================
*/
void 
SpeStringrstrim(SpeString_t* str) {
  ASSERT(str);
  int pos;
  for (pos = str->len-1; pos >= 0; pos--) {
    if (str->data[pos] == '\t' || str->data[pos] == ' ' ||
        str->data[pos] == '\r' || str->data[pos] == '\n') continue;
    break;
  }
  str->len            = pos + 1;
  str->data[str->len] = 0;
}

/*
===================================================================================================
SpeString_split
    split spe_string into SpeSlist_t
===================================================================================================
*/
SpeSlist_t*
SpeString_split(SpeString_t* str, const char* key) {
  ASSERT(str && key);
  SpeSlist_t* slist = spe_slist_create();
  if (!slist) {
    SPE_LOG_ERR("spe_slist_create error");
    return NULL;
  }
  // split from left to right
  int start = 0;
  while (start < str->len) {
    // split one by one
    char* point = strstr(str->data + start, key);
    if (!point) break;
    // add to string ignore null
    if (point != str->data + start) {
      spe_slist_appendb(slist, str->data + start, point - str->data - start);
    }
    start = point - str->data + strlen(key);
  }
  if (str->len != start) {
    spe_slist_appendb(slist, str->data + start, str->len - start);  
  }
  return slist;
}

/*
===================================================================================================
SpeStringCmp
  compare two spe_string
===================================================================================================
*/
int 
SpeStringCmp(SpeString_t* str1, SpeString_t* str2) {
  ASSERT(str1 && str2);
  int minlen = (str1->len > str2->len) ? str2->len : str1->len;
  int ret = memcmp(str1->data, str2->data, minlen);
  if (ret) return ret;
  // length is equal  
  if (str1->len == str2->len) return 0;
  if (str1->len > str2->len) return 1;
  return -1;
}

/*
===================================================================================================
SpeString_tolower
  change spe_string to lower
===================================================================================================
*/
void
SpeString_tolower(SpeString_t* str) {
  ASSERT(str);
  for (int i=0; i<str->len; i++) {
    if (str->data[i]>='A' && str->data[i]<='Z') str->data[i] += 32;
  }
}

/*
===================================================================================================
SpeString_toupper
  change spe_string to capitable
===================================================================================================
*/
void
SpeString_toupper(SpeString_t* str) {
  ASSERT(str);
  for (int i=0; i <str->len; i++) {
    if (str->data[i]>='a' && str->data[i]<='z') str->data[i] -= 32;
  }
}

/*
===================================================================================================
SpeStringread
===================================================================================================
*/
int
SpeStringread(int fd, SpeString_t* str, unsigned len) {
  ASSERT(str);
  if (!SpeStringready(str, len+1)) {
    SPE_LOG_ERR("SpeStringready error");
    return -1;
  }
  int res = read(fd, str->_start, len);
  if (res <= 0) return res;
  str->len            = res;
  str->data           = str->_start;
  str->data[str->len] = 0;
  return res;
}

/*
===================================================================================================
SpeStringread_append
===================================================================================================
*/
int
SpeStringread_append(int fd, SpeString_t* str, unsigned len) {
  ASSERT(str);
  if (!SpeStringreadyplus(str, len+1)) {
    SPE_LOG_ERR("SpeStringreadyplus error");
    return -1;
  }
  int res = read(fd, str->data+str->len, len);
  if (res <= 0) return res;
  str->len            += res;
  str->data[str->len] = 0;
  return res;
}

#define DEFAULT_SIZE  128

/*
===================================================================================================
SpeStringCreate
===================================================================================================
*/
SpeString_t* 
SpeStringCreate(unsigned defaultSize) {
  if (!defaultSize) defaultSize = DEFAULT_SIZE;
  SpeString_t* str = calloc(1, sizeof(SpeString_t)+defaultSize*sizeof(char));
  if (!str) {
    SPE_LOG_ERR("SpeString_t calloc error");
    return NULL;
  }
  str->_start = str->_buffer;
  str->data   = str->_start;
  str->_size  = defaultSize;
  return str;
}

/*
===================================================================================================
SpeStringDestroy
    free spe_string
===================================================================================================
*/
void
SpeStringDestroy(SpeString_t* str) {
  if (!str) return;
  if (str->_start != str->_buffer) free(str->_start);
  free(str);
}

/*
===================================================================================================
spe_slist_ready
  spe_slist ready
===================================================================================================
*/
static bool 
spe_slist_ready(SpeSlist_t* slist, unsigned need_size) {
  // have enough space
  unsigned size = slist->_size;
  if (need_size <= size) return true;
  // realloc space
  slist->_size = 30 + need_size + (need_size >> 3);
  SpeString_t** new_data;
  new_data = realloc(slist->data, slist->_size*sizeof(SpeString_t*));
  if (!new_data) {
    SPE_LOG_ERR("realloc error");
    slist->_size = size;
    return false;
  }
  slist->data = new_data;
  // clean other
  for (int i = slist->len; i < slist->_size; i++) slist->data[i] = NULL;
  return true;
}

/*
===================================================================================================
spe_slist_readyplus
  call spe_slist
===================================================================================================
*/
static inline bool 
spe_slist_readyplus(SpeSlist_t* slist, unsigned n) {
  return spe_slist_ready(slist, slist->len + n);
}
 
/*
===================================================================================================
spe_slist_appendb
  add new string in strList
===================================================================================================
*/
bool 
spe_slist_appendb(SpeSlist_t* slist, char* str, unsigned len) {
  ASSERT(slist && str);
  if (!spe_slist_readyplus(slist, 1)) {
    SPE_LOG_ERR("spe_slist_readyplus error");
    return false;
  }
  // copy string
  if (!slist->data[slist->len]) {
    slist->data[slist->len] = SpeStringCreate(80);
    if (!slist->data[slist->len]) return false;
  }
  SpeStringCopyb(slist->data[slist->len], str, len);
  slist->len++;
  return true;
}
