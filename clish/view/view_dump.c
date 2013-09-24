/*
 * view_dump.c
 */
#include "private.h"
#include "lub/dump.h"

/*--------------------------------------------------------- */
void clish_view_dump(clish_view_t * this)
{
	clish_command_t *c;
	lub_bintree_iterator_t iter;
	unsigned i;

	lub_dump_printf("view(%p)\n", this);
	lub_dump_indent();

	lub_dump_printf("name  : %s\n", clish_view__get_name(this));
	lub_dump_printf("depth : %u\n", clish_view__get_depth(this));

	/* Get each namespace to dump their details */
	for (i = 0; i < this->nspacec; i++) {
		clish_nspace_dump(clish_view__get_nspace(this, i));
	}

	/* iterate the tree of commands */
	c = lub_bintree_findfirst(&this->tree);
	for (lub_bintree_iterator_init(&iter, &this->tree, c);
		c; c = lub_bintree_iterator_next(&iter)) {
		clish_command_dump(c);
	}

	lub_dump_undent();
}

/*--------------------------------------------------------- */
