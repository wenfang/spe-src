#ifndef __SPE_MAIN_H
#define __SPE_MAIN_H

#include <stdbool.h>

struct spe_module_s {
  bool              (*init)(void);
  bool              (*exit)(void);
  bool              (*start)(void);
  bool              (*end)(void);
  void              (*before_loop)(void);
  void              (*after_loop)(void);
};
typedef struct spe_module_s spe_module_t;

extern void
spe_register_module(spe_module_t*);

extern bool spe_main_stop;

#endif
