#ifdef DEBUG

/*
 * view_dump.c
 */
#include "private.h"
#include "lub/dump.h"
#include "lub/list.h"

/*--------------------------------------------------------- */
void clish_view_dump(clish_view_t * this)
{
	clish_command_t *c;
	lub_list_node_t *iter;
	lub_bintree_iterator_t iterc;
	clish_nspace_t *nspace;

	lub_dump_printf("view(%p)\n", this);
	lub_dump_indent();

	lub_dump_printf("name   : %s\n", clish_view__get_name(this));
	lub_dump_printf("depth  : %u\n", clish_view__get_depth(this));
	lub_dump_printf("access : %u\n", clish_view__get_access(this));

	/* Get each namespace to dump their details */
	for(iter = lub_list__get_head(this->nspaces);
		iter; iter = lub_list_node__get_next(iter)) {
		nspace = (clish_nspace_t *)lub_list_node__get_data(iter);
		clish_nspace_dump(nspace);
	}

	/* Iterate the tree of commands */
	c = lub_bintree_findfirst(&this->tree);
	for (lub_bintree_iterator_init(&iterc, &this->tree, c);
		c; c = lub_bintree_iterator_next(&iterc)) {
		clish_command_dump(c);
	}

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
