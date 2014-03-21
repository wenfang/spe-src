#include "spe_routine.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>

static spe_routine_t  main_routine;
static spe_routine_t* curr_routine = &main_routine;

static void
routine_main(spe_routine_t* routine) {
  SPE_HANDLER_CALL(routine->handler);
  routine->finished = 1;
  curr_routine = routine->prev;
  setcontext(&curr_routine->context);
}

/*
===================================================================================================
spe_routine_set_handler
===================================================================================================
*/
void
spe_routine_set_handler(spe_routine_t* routine, spe_handler_t handler) {
  ASSERT(routine);
  routine->handler = handler;
}

/*
===================================================================================================
spe_routine_create
===================================================================================================
*/
spe_routine_t*
spe_routine_create(void) {
  spe_routine_t* routine = calloc(1, sizeof(spe_routine_t));
  if (!routine) {
    SPE_LOG_ERR("routine calloc error");
    return NULL;
  }
  getcontext(&routine->context);
  routine->context.uc_stack.ss_sp   = routine->stack;
  routine->context.uc_stack.ss_size = sizeof(routine->stack);
  routine->handler                  = SPE_HANDLER_NULL;
  makecontext(&routine->context, (void(*)(void))routine_main, 1, routine);
  return routine;
}

/*
===================================================================================================
spe_routine_destroy
===================================================================================================
*/
void
spe_routine_destroy(spe_routine_t* routine) {
  ASSERT(routine);
  free(routine);
}

/*
===================================================================================================
spe_routine_resume
===================================================================================================
*/
void
spe_routine_resume(spe_routine_t* routine) {
  ASSERT(routine);
  routine->prev = curr_routine;
  curr_routine = routine;
  swapcontext(&routine->prev->context, &routine->context);
  if (routine->finished) spe_routine_destroy(routine);
}

/*
===================================================================================================
spe_routine_yield
===================================================================================================
*/
void
spe_routine_yield() {
  ASSERT(curr_routine);
  spe_routine_t* routine = curr_routine;
  curr_routine = routine->prev;
  swapcontext(&routine->context, &curr_routine->context);
}
