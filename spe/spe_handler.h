#ifndef __SPE_HANDLER_H
#define __SPE_HANDLER_H

typedef void (*spe_Handler0)(void);
typedef void (*spe_Handler1)(void*);
typedef void (*spe_Handler2)(void*, void*);
typedef void (*spe_Handler3)(void*, void*, void*);

struct spe_handler_s {
  void*     handler;
  void*     arg1;
  void*     arg2;
  void*     arg3;
  unsigned  argc;
};
typedef struct spe_handler_s spe_handler_t;

#define SPE_HANDLER_NULL                        (spe_handler_t){NULL, NULL, NULL, NULL, 0}
#define SPE_HANDLER0(handler)                   (spe_handler_t){handler, NULL, NULL, NULL, 0}
#define SPE_HANDLER1(handler, arg1)             (spe_handler_t){handler, arg1, NULL, NULL, 1}
#define SPE_HANDLER2(handler, arg1, arg2)       (spe_handler_t){handler, arg1, arg2, NULL, 2}
#define SPE_HANDLER3(handler, arg1, arg2, arg3) (spe_handler_t){handler, arg1, arg2, arg3, 3}

#define SPE_HANDLER_CALL(h)                                         \
  do {                                                              \
    if (!(h).handler) break;                                        \
    switch ((h).argc) {                                             \
    case 0:                                                         \
      ((spe_Handler0)((h).handler))();                              \
      break;                                                        \
    case 1:                                                         \
      ((spe_Handler1)((h).handler))((h).arg1);                      \
      break;                                                        \
    case 2:                                                         \
      ((spe_Handler2)((h).handler))((h).arg1, (h).arg2);            \
      break;                                                        \
    case 3:                                                         \
      ((spe_Handler3)((h).handler))((h).arg1, (h).arg2, (h).arg3);  \
      break;                                                        \
    }                                                               \
  } while(0)

#endif
