#include "spe_io.h"
#include "spe_opt.h"
#include "spe_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#define SEC_MAXLEN  16
#define KEY_MAXLEN  16
#define VAL_MAXLEN  128

struct spe_opt_s {
	char                sec[SEC_MAXLEN];
 	char                key[KEY_MAXLEN];
	char                val[VAL_MAXLEN];      // string value
	struct list_head    node;
};
typedef struct spe_opt_s spe_opt_t;

static LIST_HEAD(options);

/*
===================================================================================================
spe_opt_set
===================================================================================================
*/
static bool 
spe_opt_set(char* sec, char* key, char* val) {
  spe_opt_t* opt = calloc (1, sizeof(spe_opt_t));
  if (!opt) return false;
  // set section and key 
  strncpy(opt->sec, sec, SEC_MAXLEN);
  strncpy(opt->key, key, KEY_MAXLEN);
  strncpy(opt->val, val, VAL_MAXLEN);
  // add to options list
  INIT_LIST_HEAD(&opt->node);
  list_add_tail(&opt->node, &options);
  return true;
}

/*
===================================================================================================
spe_opt_int
===================================================================================================
*/
int
spe_opt_int(char* sec, char* key, int defval) {
  if (!key) return defval;
  if (!sec) sec = "global";
  spe_opt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(sec, entry->sec) && !strcmp(key, entry->key)) {
      return atoi(entry->val);
    }
  }
  return defval;
}

/*
===================================================================================================
spe_opt_string
===================================================================================================
*/
const char* 
spe_opt_string(char* sec, char* key, const char* defval) {
  if (!key) return defval;
  if (!sec) sec = "global";
  spe_opt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(sec, entry->sec) && !strcmp(key, entry->key)) {
      return entry->val;
    }
  }
  return defval;
}

/*
===================================================================================================
spe_opt_parse_file
    parse conf file
===================================================================================================
*/
bool 
spe_opt_create(const char* fname) {
  char sec[SEC_MAXLEN];
  char key[KEY_MAXLEN];
  char val[VAL_MAXLEN];

  spe_io_t* io = spe_io_create(fname);
  if (!io) return false;
  // set default section
  strcpy(sec, "global");
  while (1) {
    // get one line from file
    if (spe_io_readuntil(io, "\n") <= 0) break;
    spe_string_t* line = io->data;
    spe_string_strim(line);
    // ignore empty and comment line
    if (line->len == 0 || line->data[0] == '#') continue;
    // section line, get section
    if (line->data[0] == '[' && line->data[line->len-1] == ']') {
      spe_string_consume(line, 1);
      spe_string_rconsume(line, 1);
      spe_string_strim(line);
      // section can't be null
      if (line->len == 0) {
        spe_io_destroy(io);
        return false;
      }
      strncpy(sec, line->data, SEC_MAXLEN);
      sec[SEC_MAXLEN-1] = 0;
      continue;
    }
    // split key and value
    spe_slist_t* slist = spe_string_split(line, "=");
    if (slist->len != 2) {
      spe_slist_destroy(slist);
      spe_io_destroy(io);
      return false;
    }
    spe_string_t* key_str = slist->data[0];
    spe_string_t* val_str = slist->data[1];
    spe_string_strim(key_str);
    spe_string_strim(val_str);
    strncpy(key, key_str->data, KEY_MAXLEN);
    key[KEY_MAXLEN-1] = 0;
    strncpy(val, val_str->data, VAL_MAXLEN);
    val[VAL_MAXLEN-1] = 0;
    spe_slist_destroy(slist);
    // set option value
    spe_opt_set(sec, key, val);
  }
  spe_io_destroy(io);
  return true;
}

/*
===================================================================================================
spe_opt_destroy
===================================================================================================
*/
void
spe_opt_destroy() {
  spe_opt_t* entry;
  spe_opt_t* tmp;
  list_for_each_entry_safe(entry, tmp, &options, node) {
    list_del_init(&entry->node);
    free(entry);
  }
}
