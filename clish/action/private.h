/*
 * action.h
 */

#include "clish/action.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_action_s {
	char *script;
	clish_sym_t *builtin;
	char *shebang;
};
