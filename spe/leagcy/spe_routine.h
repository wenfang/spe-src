#ifndef __SPE_ROUTINE_H
#define __SPE_ROUTINE_H

#include "spe_handler.h"
#include <ucontext.h>

#define STACK_SIZE  8192

struct spe_routine_s {
  ucontext_t            context;
  char                  stack[STACK_SIZE];
  spe_handler_t         handler;
  struct spe_routine_s  *prev;
  unsigned              finished:1;
};
typedef struct spe_routine_s spe_routine_t;

extern void
spe_routine_set_handler(spe_routine_t* routine, spe_handler_t handler);

extern spe_routine_t*
spe_routine_create(void);

extern void
spe_routine_destroy(spe_routine_t* routine);

extern void
spe_routine_resume(spe_routine_t* routine);

extern void
spe_routine_yield(void);

#endif
