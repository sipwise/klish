/*
 * action.h
 */
#ifndef _clish_action_h
#define _clish_action_h

typedef struct clish_action_s clish_action_t;

#include "lub/bintree.h"
#include "clish/plugin.h"

/*=====================================
 * ACTION INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_action_t *clish_action_new(void);

/*-----------------
 * methods
 *----------------- */
void clish_action_delete(clish_action_t *instance);
void clish_action_dump(const clish_action_t *instance);

/*-----------------
 * attributes
 *----------------- */
void clish_action__set_script(clish_action_t *instance, const char *script);
char *clish_action__get_script(const clish_action_t *instance);
void clish_action__set_builtin(clish_action_t *instance, clish_sym_t *builtin);
clish_sym_t *clish_action__get_builtin(const clish_action_t *instance);
void clish_action__set_shebang(clish_action_t *instance, const char *shebang);
const char *clish_action__get_shebang(const clish_action_t *instance);

#endif				/* _clish_action_h */
