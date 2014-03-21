#ifndef __SPE_OBJECT_H 
#define __SPE_OBJECT_H

#include "spe_list.h"
#include <stdbool.h>
#include <pthread.h>

#define SPE_OBJECT_FREE   0
#define SPE_OBJECT_QUEUE  1
#define SPE_OBJECT_RUN    2

struct spe_msg_s {
  unsigned          type;
  void*             content;
  struct list_head  msg_node;
};
typedef struct spe_msg_s spe_msg_t;

struct spe_object_s;
typedef void (*spe_object_Handler)(struct spe_object_s*, struct spe_msg_s*);

#define spe_object_HEAD               \
  unsigned long       id;             \
  unsigned            status;         \
  spe_object_Handler  handler;        \
  pthread_mutex_t     msg_lock;       \
  struct list_head    msg_head;       \
  struct list_head    worker_node;

struct spe_object_s {
  spe_object_HEAD
};
typedef struct spe_object_s spe_object_t;

extern void
spe_object_init(spe_object_t* obj, unsigned long id, spe_object_Handler Handler);

extern bool
spe_object_send_msg(unsigned long id, unsigned type, void* content);

#endif
