#ifdef DEBUG

/*
 * bintree_dump.c
 */

#include <stdio.h>
#include "private.h"

/*--------------------------------------------------------- */
void _lub_bintree_dump(lub_bintree_t * this, lub_bintree_node_t * node)
{
	if (node->left) {
		_lub_bintree_dump(this, node->left);
	}
	printf(" %s%p",
	       (this->root == node) ? "(R)" : "",
	       lub_bintree_getclientnode(this, node));
	if (node->right) {
		_lub_bintree_dump(this, node->right);
	}
}

/*--------------------------------------------------------- */
void lub_bintree_dump(lub_bintree_t * this)
{
	if (this->root) {
		_lub_bintree_dump(this, this->root);
	}
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
