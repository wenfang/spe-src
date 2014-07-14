#include "spe_map.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define KEY_MAXLEN 16

struct spe_map_item_s {
  char              _key[KEY_MAXLEN];
  void*             _obj;
  spe_map_Handler   _handler;
  struct hlist_node _node;
};
typedef struct spe_map_item_s spe_map_item_t;

/*
===================================================================================================
spe_map_item_create
===================================================================================================
*/
static spe_map_item_t*
spe_map_item_create(const char* key, void* obj, spe_map_Handler handler) {
  spe_map_item_t* item = calloc(1, sizeof(spe_map_item_t));
  if (!item) return NULL;
  strncpy(item->_key, key, KEY_MAXLEN-1);
  item->_key[KEY_MAXLEN-1] = 0;
  item->_obj      = obj;
  item->_handler  = handler;
  INIT_HLIST_NODE(&item->_node);
  return item;
}

/*
===================================================================================================
spe_map_item_destroy
===================================================================================================
*/
static inline void 
spe_map_item_destroy(spe_map_item_t* item) {
  if (!item) return;
  if (item->_handler) item->_handler(item->_obj);
  free(item);
}

/*
===================================================================================================
 gen hash value, copy from redis
===================================================================================================
*/
static unsigned int genhashvalue(const void* key, int len)
{
  uint32_t seed = 5381;
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  /* Mix 4 bytes at a time into the hash */
  const unsigned char *data = (const unsigned char *)key;

  while (len >= 4) {
    uint32_t k = *(uint32_t*)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  /* Handle the last few bytes of the input array  */
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few 
     bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return (unsigned int)h;
}

/*
===============================================================================
mjmap_Search
===============================================================================
*/
static spe_map_item_t*
spe_map_search(spe_map_t* map, const char* key) {
  unsigned int hashvalue = genhashvalue((void*)key, strlen(key));
  unsigned int index = hashvalue % map->size;
  // search entry
  spe_map_item_t* item = NULL;
  struct hlist_node* entry;
  hlist_for_each_entry(item, entry, &map->hashHead[index], _node) { 
    if (strcmp(item->_key, key) == 0) return item;
  }
  return NULL;
}

/*
===================================================================================================
spe_map_get
===================================================================================================
*/
void*
spe_map_get(spe_map_t* map, const char* key) {
  if (!map || !key) return NULL;
  spe_map_item_t* item = spe_map_search(map, key);
  if (!item) return NULL;
  return item->_obj;
}

/*
===================================================================================================
spe_map_set
===================================================================================================
*/
int 
spe_map_set(spe_map_t* map, const char* key, void* obj, spe_map_Handler handler) {
  if (!map || !key) return -1;
  unsigned int hashvalue = genhashvalue((void*)key, strlen(key));
  unsigned int index = hashvalue % map->size;
  spe_map_item_t* item = spe_map_search(map, key);
  if (item) return -2;
  item = spe_map_item_create(key, obj, handler);
  if (!item) return -1;
  // add to list and elem list
  hlist_add_head(&item->_node, &map->hashHead[index]);
  return 0;
}

/*
===================================================================================================
spe_map_del
===================================================================================================
*/
bool 
spe_map_del(spe_map_t* map, const char* key) {
  if (!map || !key) return false;
  spe_map_item_t* item = spe_map_search(map, key);
  if (!item) return false;
  hlist_del(&item->_node);
  spe_map_item_destroy(item);
  return true;
}

/*
===================================================================================================
spe_map_clean
===================================================================================================
*/
void
spe_map_clean(spe_map_t* map) {
  if (!map) return;
	spe_map_item_t* item;
  struct hlist_node* entry;
  struct hlist_node* tmp;
  for (int i=0; i<map->size; i++) {
    hlist_for_each_entry_safe(item, entry, tmp, &map->hashHead[i], _node) { 
		  hlist_del(&item->_node);
		  spe_map_item_destroy(item);
	  }
  }
}

/*
===================================================================================================
spe_map_create
===================================================================================================
*/
spe_map_t*
spe_map_create(unsigned mapsize) {
  ASSERT(mapsize);
  spe_map_t* map = calloc(1, sizeof(spe_map_t) + mapsize*sizeof(struct hlist_head));
  if (!map) {
    SPE_LOG_ERR("map calloc error");
    return NULL;
  }
  map->size = mapsize;
  for (int i=0; i<mapsize; i++) INIT_HLIST_HEAD(&map->hashHead[i]);
  return map;
}

/*
===================================================================================================
spe_map_destroy
===================================================================================================
*/
void
spe_map_destroy(spe_map_t* map) {
  spe_map_clean(map);
  free(map);
}
