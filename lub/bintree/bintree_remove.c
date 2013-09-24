/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_remove.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation removes a "node" from the specified "tree"
 *
 * tree       - the "tree" instance to invoke this operation upon
 * clientnode - the node to remove
 *
 * RETURNS
 *   none
 *
 * POST CONDITIONS
 * The "node" will no longer be part of the specified tree, and can be
 * subsequently re-inserted
 *
 * If the node is not present in the specified tree, then an assert
 * will fire.
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:06:58 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 27-Dec-2004		Graeme McKerrell	
 *    added assert to catch removal of invalid node 
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
  * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 23-Mar-2004		Graeme McKerrell	
 *    fixed to ensure that control block is re-initialised.
 * 16-Mar-2004		Graeme McKerrell	
 *    removed assert.
 * 27-Feb-2004		Graeme McKerrell	
 *    removed spurious call to node_init
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use new node,key comparison ordering
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
void lub_bintree_remove(lub_bintree_t * this, void *clientnode)
{
	lub_bintree_node_t *x, *t;
	lub_bintree_key_t key;
	int comp;

	/* get the key from the node */
	this->getkeyFn(clientnode, &key);

	/* bring the node in question to the root of the tree */
	t = lub_bintree_splay(this, this->root, &key);

	/* check that the node was there to remove */
	comp = lub_bintree_compare(this, t, &key);

	assert(0 == comp);
	if (0 == comp) {
		if (t->left == NULL) {
			x = t->right;
		} else {
			x = lub_bintree_splay(this, t->left, &key);
			x->right = t->right;
		}
		/* set the new root */
		this->root = x;

		/* re-initialise the node control block for re-use */
		lub_bintree_node_init(lub_bintree_getnode(this, clientnode));
	}
}

/*--------------------------------------------------------- */
