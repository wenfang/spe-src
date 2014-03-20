#ifndef __SPE_MODULE_H
#define __SPE_MODULE_H

#include "spe_list.h"
#include <stdbool.h>

struct spe_module_s {
  bool              (*init)(void);
  bool              (*exit)(void);
  bool              (*start)(void);
  bool              (*end)(void);
};
typedef struct spe_module_s spe_module_t;

extern void 
spe_register_module(spe_module_t*);

extern bool
spe_modules_init(void);

extern bool
spe_modules_exit(void);

extern bool
spe_modules_start(void);

extern bool
spe_modules_end(void);

#endif
