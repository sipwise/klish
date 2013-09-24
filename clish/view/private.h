/*
 * view.h
 */
#include "clish/view.h"
#include "lub/bintree.h"
#include "clish/hotkey.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_view_s {
	lub_bintree_t tree;
	lub_bintree_node_t bt_node;
	char *name;
	char *prompt;
	unsigned int nspacec;
	clish_nspace_t **nspacev;
	clish_hotkeyv_t *hotkeys;
	unsigned int depth;
	clish_view_restore_t restore;
};
