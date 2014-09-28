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

struct speOpt_s {
	char              sec[SEC_MAXLEN];
 	char              key[KEY_MAXLEN];
	char              val[VAL_MAXLEN];      // string value
	struct list_head  node;
};
typedef struct speOpt_s speOpt_t;

static LIST_HEAD(options);

/*
===================================================================================================
speOptSet
===================================================================================================
*/
static bool 
speOptSet(char* sec, char* key, char* val) {
  speOpt_t* opt = calloc (1, sizeof(speOpt_t));
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
SpeOptInt
===================================================================================================
*/
int
SpeOptInt(char* section, char* key, int defaultValue) {
  if (!key) return defaultValue;
  if (!section) section = "global";
  speOpt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return atoi(entry->val);
    }
  }
  return defaultValue;
}

/*
===================================================================================================
SpeOptString
===================================================================================================
*/
const char* 
SpeOptString(char* section, char* key, const char* defaultValue) {
  if (!key) return defaultValue;
  if (!section) section = "global";
  speOpt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return entry->val;
    }
  }
  return defaultValue;
}

/*
===================================================================================================
speOptCreate
    parse conf file
===================================================================================================
*/
bool 
speOptCreate(const char* configFile) {
  char sec[SEC_MAXLEN];
  char key[KEY_MAXLEN];
  char val[VAL_MAXLEN];

  SpeIo_t* io = SpeIoCreate(configFile);
  if (!io) return false;
  // set default section
  strcpy(sec, "global");
  while (1) {
    // get one line from file
    if (SpeIoReaduntil(io, "\n") <= 0) break;
    SpeString_t* line = io->Buffer;
    SpeStringStrim(line);
    // ignore empty and comment line
    if (line->len == 0 || line->data[0] == '#') continue;
    // section line, get section
    if (line->data[0] == '[' && line->data[line->len-1] == ']') {
      SpeStringConsume(line, 1);
      SpeStringRconsume(line, 1);
      SpeStringStrim(line);
      // section can't be null
      if (line->len == 0) {
        SpeIoDestroy(io);
        return false;
      }
      strncpy(sec, line->data, SEC_MAXLEN);
      sec[SEC_MAXLEN-1] = 0;
      continue;
    }
    // split key and value
    SpeSlist_t* slist = SpeStringSplit(line, "=");
    if (slist->len != 2) {
      spe_slist_destroy(slist);
      SpeIoDestroy(io);
      return false;
    }
    SpeString_t* key_str = slist->data[0];
    SpeStringStrim(key_str);
    strncpy(key, key_str->data, KEY_MAXLEN);
    key[KEY_MAXLEN-1] = 0;

    SpeString_t* val_str = slist->data[1];
    SpeStringStrim(val_str);
    strncpy(val, val_str->data, VAL_MAXLEN);
    val[VAL_MAXLEN-1] = 0;
    spe_slist_destroy(slist);
    // set option value
    speOptSet(sec, key, val);
  }
  SpeIoDestroy(io);
  return true;
}

/*
===================================================================================================
speOptDestroy
===================================================================================================
*/
void
speOptDestroy() {
  speOpt_t *entry, *tmp;
  list_for_each_entry_safe(entry, tmp, &options, node) {
    list_del_init(&entry->node);
    free(entry);
  }
}
