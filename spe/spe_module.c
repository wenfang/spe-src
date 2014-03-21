#include "spe_module.h"

static LIST_HEAD(spe_modules);

/*
===================================================================================================
spe_modules_init
===================================================================================================
*/
bool
spe_modules_init(void) {
  spe_module_t* mod;
  bool res = true;
  list_for_each_entry(mod, &spe_modules, node) {
    if (mod->init) res = mod->init();
    if (!res) break;
  }
  return res;
}

/*
===================================================================================================
spe_modules_exit
===================================================================================================
*/
bool
spe_modules_exit(void) {
  spe_module_t* mod;
  bool res = true;
  list_for_each_entry(mod, &spe_modules, node) {
    if (mod->exit) res = mod->exit();
    if (!res) break;
  }
  return res;
}

/*
===================================================================================================
spe_modules_start
===================================================================================================
*/
bool
spe_modules_start(void) {
  spe_module_t* mod;
  bool res = true;
  list_for_each_entry(mod, &spe_modules, node) {
    if (mod->start) res = mod->start();
    if (!res) break;
  }
  return res;
}

/*
===================================================================================================
spe_module_end
===================================================================================================
*/
bool
spe_modules_end(void) {
  spe_module_t* mod;
  bool res = true;
  list_for_each_entry(mod, &spe_modules, node) {
    if (mod->end) res = mod->end();
    if (!res) break;
  }
  return res;
}

/*
===================================================================================================
spe_register_module
===================================================================================================
*/
void
spe_register_module(spe_module_t* mod) {
  if (!mod) return;
  list_add_tail(&mod->node, &spe_modules);
}
