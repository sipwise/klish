/*
 * var/private.h
 */
#include "clish/var.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */

struct clish_var_s {
	lub_bintree_node_t bt_node;
	char *name;
	bool_t dynamic;
	char *value;
	char *saved; /* Saved value of static variable */
	clish_action_t *action;
};
