#include "spe_objmap.h"
#include "spe_list.h"
#include "spe_util.h"
#include "spe_log.h"
#include <pthread.h>

#define OBJMAP_SIZE 999983

static pthread_rwlock_t   objmap_lock;
static struct hlist_head  objmap_head[OBJMAP_SIZE];

struct spe_objmap_item_s {
  spe_object_t*     _obj;
  struct hlist_node _node;
};
typedef struct spe_objmap_item_s spe_objmap_item_t;

static spe_objmap_item_t*
spe_objmap_item_create() {
  spe_objmap_item_t* item = calloc(1, sizeof(spe_objmap_item_t));
  if (!item) {
    SPE_LOG_ERR("item calloc error");
    return NULL;
  }
  INIT_HLIST_NODE(&item->_node);
  return item;
}

static inline void
spe_objmap_item_destroy(spe_objmap_item_t* item) {
  free(item);
}

static spe_objmap_item_t*
spe_objmap_get_item(unsigned long id) {
  unsigned long key = id%OBJMAP_SIZE;
  spe_objmap_item_t* item = NULL;
  spe_objmap_item_t* tmp;
  struct hlist_node* entry;
  pthread_rwlock_rdlock(&objmap_lock);
  hlist_for_each_entry(tmp, entry, &objmap_head[key], _node) {
    if (id == tmp->_obj->id) {
      item = tmp;
      break;
    }
  }
  pthread_rwlock_unlock(&objmap_lock);
  return item;
}

/*
===================================================================================================
spe_objmap_add
===================================================================================================
*/
bool
spe_objmap_add(spe_object_t* obj) {
  ASSERT(obj);
  spe_objmap_item_t* item = spe_objmap_get_item(obj->id);
  if (item) return false;
  item = spe_objmap_item_create();
  if (!item) {
    SPE_LOG_ERR("objmap_item_create error");
    return false;
  }
  item->_obj = obj;
  unsigned long key = obj->id%OBJMAP_SIZE;
  pthread_rwlock_wrlock(&objmap_lock);
  hlist_add_head(&item->_node, &objmap_head[key]);
  pthread_rwlock_unlock(&objmap_lock);
  return true;
}

/*
===================================================================================================
spe_objmap_del
===================================================================================================
*/
bool
spe_objmap_del(unsigned long id) {
  spe_objmap_item_t* item = spe_objmap_get_item(id);
  if (!item) return false;
  pthread_rwlock_wrlock(&objmap_lock);
  hlist_del(&item->_node);
  pthread_rwlock_unlock(&objmap_lock);
  spe_objmap_item_destroy(item);
  return true;
}

/*
===================================================================================================
spe_objmap_get
===================================================================================================
*/
spe_object_t*
spe_objmap_get(unsigned long id) {
  spe_objmap_item_t* item = spe_objmap_get_item(id);
  if (!item) return NULL;
  return item->_obj;
}

__attribute__((constructor))
static void
objmap_init(void) {
  for (int i=0; i<OBJMAP_SIZE; i++) {
    INIT_HLIST_HEAD(&objmap_head[i]);
  }
  pthread_rwlock_init(&objmap_lock, NULL);
}
