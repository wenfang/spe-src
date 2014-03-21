#include "spe_object.h"
#include "spe_objmap.h"
#include "spe_worker.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>

/*
===================================================================================================
spe_object_init
===================================================================================================
*/
void
spe_object_init(spe_object_t* obj, unsigned long id, spe_object_Handler Handler) {
  ASSERT(obj);
  obj->id       = id;
  obj->status   = SPE_OBJECT_FREE;
  obj->handler  = Handler;
  pthread_mutex_init(&obj->msg_lock, NULL);
  INIT_LIST_HEAD(&obj->msg_head);
  INIT_LIST_HEAD(&obj->worker_node);
  spe_objmap_add(obj);
}

/*
===================================================================================================
spe_object_send_msg
===================================================================================================
*/
bool
spe_object_send_msg(unsigned long id, unsigned type, void* content) {
  spe_object_t* obj = spe_objmap_get(id);
  if (!obj) return false;
  spe_msg_t* msg = calloc(1, sizeof(spe_msg_t));
  if (!msg) {
    SPE_LOG_ERR("msg calloc error");
    return false;
  }
  msg->type     = type;
  msg->content  = content;
  INIT_LIST_HEAD(&msg->msg_node);
  pthread_mutex_lock(&obj->msg_lock);
  list_add_tail(&msg->msg_node, &obj->msg_head);
  pthread_mutex_unlock(&obj->msg_lock);
  spe_worker_do(obj);
  return true;
}
