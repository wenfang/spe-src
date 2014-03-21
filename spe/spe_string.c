#include "spe_string.h"
#include "spe_log.h"

/*
===================================================================================================
spe_string_ready
  alloc enough size for spe_string
===================================================================================================
*/
static bool 
spe_string_ready(spe_string_t* str, unsigned need_size) {
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
spe_string_readyplus
  call spe_string_ready
===================================================================================================
*/
static inline bool 
spe_string_readyplus(spe_string_t* str, unsigned need_size_plus) {
  return spe_string_ready(str, str->len + need_size_plus);
}

/*
===================================================================================================
spe_string_catb
  cat binary string to spe_string
===================================================================================================
*/
bool 
spe_string_catb(spe_string_t* dst, const char* src, unsigned len) {
  ASSERT(dst && src);
  if (!spe_string_readyplus(dst, len+1)) {
    SPE_LOG_ERR("spe_string_readyplus error");
    return false;
  }
  memcpy(dst->data + dst->len, src, len);            
  dst->len            += len;
  dst->data[dst->len] = 0;                
  return true;
}

/*
===================================================================================================
spe_string_copyb
  copy binary string to spe_string
===================================================================================================
*/
bool 
spe_string_copyb(spe_string_t* dst, const char* src, unsigned len) {
  ASSERT(dst && src);
  if (!spe_string_ready(dst, len+1)) {
    SPE_LOG_ERR("spe_string_ready error");
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
spe_string_consume
  consume len string from left
===================================================================================================
*/
int 
spe_string_consume(spe_string_t* str, unsigned len) {
  ASSERT(str);
  if (len >= str->len) {
    len = str->len;
    spe_string_clean(str);
    return len; 
  }
  str->data           += len;
  str->len            -= len;
  str->data[str->len] = 0;
  return len;
}

/*
===================================================================================================
spe_string_rconsume
  consume str from right
===================================================================================================
*/
int 
spe_string_rconsume(spe_string_t* str, unsigned len) {
  ASSERT(str);
  if (len >= str->len) {
    len = str->len;
    spe_string_clean(str);
    return len;
  }
  str->len            -= len;
  str->data[str->len] = 0;
  return len;
}

/*
===================================================================================================
spe_string_search
    search string in x
    return startpositon in x
            -1 for no found or error
===================================================================================================
*/
int 
spe_string_search(spe_string_t* str, const char* key) {
  ASSERT(str && key);
  char* point = strstr(str->data, key);
  if (!point) return -1;
  return point - str->data;
}

/*
===================================================================================================
spe_string_lstrim
  strim string from left
===================================================================================================
*/
void 
spe_string_lstrim(spe_string_t* str) {
  ASSERT(str);
  int pos;
  for (pos = 0; pos < str->len; pos++) {
    if (str->data[pos] == '\t' || str->data[pos] == ' ' ||
        str->data[pos] == '\r' || str->data[pos] == '\n') continue;
    break;
  }
  if (pos) spe_string_consume(str, pos);
}

/*
===================================================================================================
spe_string_rstrim
  strim string from right
===================================================================================================
*/
void 
spe_string_rstrim(spe_string_t* str) {
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
spe_string_split
    split spe_string into spe_slist_t
===================================================================================================
*/
spe_slist_t*
spe_string_split(spe_string_t* str, const char* key) {
  ASSERT(str && key);
  spe_slist_t* slist = spe_slist_create();
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
spe_string_cmp
  compare two spe_string
===================================================================================================
*/
int 
spe_string_cmp(spe_string_t* str1, spe_string_t* str2) {
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
spe_string_tolower
  change spe_string to lower
===================================================================================================
*/
void
spe_string_tolower(spe_string_t* str) {
  ASSERT(str);
  for (int i=0; i<str->len; i++) {
    if (str->data[i]>='A' && str->data[i]<='Z') str->data[i] += 32;
  }
}

/*
===================================================================================================
spe_string_toupper
  change spe_string to capitable
===================================================================================================
*/
void
spe_string_toupper(spe_string_t* str) {
  ASSERT(str);
  for (int i=0; i <str->len; i++) {
    if (str->data[i]>='a' && str->data[i]<='z') str->data[i] -= 32;
  }
}

/*
===================================================================================================
spe_string_read
===================================================================================================
*/
int
spe_string_read(int fd, spe_string_t* str, unsigned len) {
  ASSERT(str);
  if (!spe_string_ready(str, len+1)) {
    SPE_LOG_ERR("spe_string_ready error");
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
spe_string_read_append
===================================================================================================
*/
int
spe_string_read_append(int fd, spe_string_t* str, unsigned len) {
  ASSERT(str);
  if (!spe_string_readyplus(str, len+1)) {
    SPE_LOG_ERR("spe_string_readyplus error");
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
spe_string_create
===================================================================================================
*/
spe_string_t* 
spe_string_create(unsigned default_size) {
  if (!default_size) default_size = DEFAULT_SIZE;
  spe_string_t* str = calloc(1, sizeof(spe_string_t)+default_size*sizeof(char));
  if (!str) {
    SPE_LOG_ERR("calloc error");
    return NULL;
  }
  str->_start = str->_buffer;
  str->data   = str->_start;
  str->_size  = default_size;
  return str;
}

/*
===================================================================================================
spe_string_destroy
    free spe_string
===================================================================================================
*/
void
spe_string_destroy(spe_string_t* str) {
  ASSERT(str);
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
spe_slist_ready(spe_slist_t* slist, unsigned need_size) {
  // have enough space
  unsigned size = slist->_size;
  if (need_size <= size) return true;
  // realloc space
  slist->_size = 30 + need_size + (need_size >> 3);
  spe_string_t** new_data;
  new_data = realloc(slist->data, slist->_size*sizeof(spe_string_t*));
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
spe_slist_readyplus(spe_slist_t* slist, unsigned n) {
  return spe_slist_ready(slist, slist->len + n);
}
 
/*
===================================================================================================
spe_slist_appendb
  add new string in strList
===================================================================================================
*/
bool 
spe_slist_appendb(spe_slist_t* slist, char* str, unsigned len) {
  ASSERT(slist && str);
  if (!spe_slist_readyplus(slist, 1)) {
    SPE_LOG_ERR("spe_slist_readyplus error");
    return false;
  }
  // copy string
  if (!slist->data[slist->len]) {
    slist->data[slist->len] = spe_string_create(80);
    if (!slist->data[slist->len]) return false;
  }
  spe_string_copyb(slist->data[slist->len], str, len);
  slist->len++;
  return true;
}
