#ifndef __SPE_MAP_H 
#define __SPE_MAP_H

#include "spe_list.h"
#include <stdbool.h>

#define SPE_KEY_MAXLEN 16

#define SPE_MAP_ERROR     -1
#define SPE_MAP_CONFLICT  -2

typedef void (*SpeMapHandler)(void*);

struct SpeMap_s {
  unsigned          size;           // element size
  SpeMapHandler     handler;
  struct list_head  listHead;       // list Head
  struct hlist_head hashHead[0];    // element of mjitem
};
typedef struct SpeMap_s SpeMap_t;

struct SpeMapItem_s {
  char              key[SPE_KEY_MAXLEN];
  void*             obj;
  struct list_head  listNode;
  struct hlist_node hashNode;
};
typedef struct SpeMapItem_s SpeMapItem_t;

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

extern SpeMapItem_t*
SpeMapNext(SpeMap_t* map, SpeMapItem_t* item);

#endif
