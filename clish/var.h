#ifndef _clish_var_h
#define _clish_var_h

#include "lub/types.h"
#include "lub/bintree.h"
#include "clish/action.h"

typedef struct clish_var_s clish_var_t;

/*=====================================
 * VAR INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
int clish_var_bt_compare(const void *clientnode, const void *clientkey);
void clish_var_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_var_bt_offset(void);
clish_var_t *clish_var_new(const char *name);
/*-----------------
 * methods
 *----------------- */
void clish_var_delete(clish_var_t *instance);
void clish_var_dump(const clish_var_t *instance);
/*-----------------
 * attributes
 *----------------- */
const char *clish_var__get_name(const clish_var_t *instance);
void clish_var__set_dynamic(clish_var_t *instance, bool_t defval);
bool_t clish_var__get_dynamic(const clish_var_t *instance);
void clish_var__set_value(clish_var_t *instance, const char *value);
char *clish_var__get_value(const clish_var_t *instance);
clish_action_t *clish_var__get_action(const clish_var_t *instance);
void clish_var__set_saved(clish_var_t *instance, const char *value);
char *clish_var__get_saved(const clish_var_t *instance);

#endif /* _clish_var_h */
