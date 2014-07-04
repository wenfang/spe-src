#ifndef __SPE_OBJMAP_H
#define __SPE_OBJMAP_H

#include "spe_object.h"
#include <stdbool.h>

extern bool
spe_objmap_add(spe_object_t* obj);

extern bool
spe_objmap_del(unsigned long id);

extern spe_object_t*
spe_objmap_get(unsigned long id);

#endif
