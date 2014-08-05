#ifndef __SPE_MAP_H 
#define __SPE_MAP_H

#include "spe_list.h"
#include <stdbool.h>

#define SPE_MAP_ERROR     -1
#define SPE_MAP_CONFLICT  -2

typedef bool (*SpeMapHandler)(void*);

struct SpeMap_s {
  unsigned          size;           // element size
  SpeMapHandler     handler;
  struct list_head  listHead;       // list Head
  struct hlist_head hashHead[0];    // element of mjitem
};
typedef struct SpeMap_s SpeMap_t;

extern SpeMap_t* 
SpeMapCreate(unsigned mapSize, SpeMapHandler handler);

extern void 
SpeMapDestroy(SpeMap_t* map);

extern void
SpeMapClean(SpeMap_t* map);

extern void* 
SpeMapGet(SpeMap_t* map, const char* key);

extern int 
SpeMapSet(SpeMap_t* map, const char* key, void* obj);

extern bool   
SpeMapDel(SpeMap_t* map, const char* key);

#endif
