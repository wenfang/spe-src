#ifndef __SPE_MAIN_H
#define __SPE_MAIN_H

#include <stdbool.h>

struct spe_module_s {
  bool  stop;
  bool  (*init)(void);
  bool  (*exit)(void);
  bool  (*start)(void);
  bool  (*end)(void);
  void  (*before_loop)(void);
  void  (*after_loop)(void);
};
typedef struct spe_module_s spe_module_t;

extern spe_module_t MainMod;

#endif
