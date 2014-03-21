#ifndef __SPE_MAP_H 
#define __SPE_MAP_H

#include "spe_list.h"
#include <stdbool.h>

typedef bool (*spe_map_Handler)(void*);

struct spe_map_s {
  unsigned          _size;          // hash length of elem
  struct hlist_head _head[0];       // element of mjitem
};
typedef struct spe_map_s spe_map_t;

extern spe_map_t* 
spe_map_create(unsigned mapsize);

extern void 
spe_map_destroy(spe_map_t* map);

extern void
spe_map_clean(spe_map_t* map);

extern bool   
spe_map_del(spe_map_t* map, const char* key);

extern void* 
spe_map_get(spe_map_t* map, const char* key);

extern int 
spe_map_set(spe_map_t* map, const char* key, void* obj, spe_map_Handler handler);

#endif
