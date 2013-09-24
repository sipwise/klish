/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_insert.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation adds a client node to the specified tree.
 *
 * tree       - the "tree" instance to invoke this operation upon
 * clientnode - a pointer to a client node. NB the tree can find the
 *              necessary util_BintreeNodeT from it's stored offset.
 *
 * RETURNS
 * 0 if the "clientnode" is added correctly to the tree.
 * 
 * If another "clientnode" already exists in the tree with the same key, then
 * -1 is returned, and the tree remains unchanged.
 *
 * If the "clientnode" had not been initialised, then an assert will fire.
 * 
 * If the bintree "node" is already part of a tree, then an assert will fire.
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:05:11 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 2-Mar-2004		Graeme McKerrell	
 *    fixed so that the insertion order is correct
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use the new getkey prototype and new node, key ordering
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include <assert.h>
#include "private.h"

/*--------------------------------------------------------- */
int lub_bintree_insert(lub_bintree_t * this, void *clientnode)
{
	int result = -1;
	lub_bintree_node_t *new;
	lub_bintree_key_t key;

	assert(clientnode);
	if (NULL != clientnode) {
		/* obtain the control block from the clientnode */
		new = lub_bintree_getnode(this, clientnode);

		/* check this node is not currently in another tree */
		assert(new->left == NULL);
		assert(new->right == NULL);

		/* add this node to the splay tree */

		/* Insert "node" into the tree , unless it's already there. */
		if (NULL == this->root) {
			this->root = new;
			this->root->left = this->root->right = NULL;
			result = 0;
		} else {
			int comp;
			/* get a key from the node */
			this->getkeyFn(clientnode, &key);

			/* do the biz */
			this->root = lub_bintree_splay(this, this->root, &key);

			/*
			 * compare the new key with the detail found
			 * in the tree
			 */
			comp = lub_bintree_compare(this, this->root, &key);
			if (comp > 0) {
				new->left = this->root->left;
				new->right = this->root;
				this->root->left = NULL;
				result = 0;
			} else if (comp < 0) {
				new->right = this->root->right;
				new->left = this->root;
				this->root->right = NULL;
				result = 0;
			} else {
				/* We get here if it's already in the tree */
			}
		}
		if (0 == result) {
			/* update the tree root */
			this->root = new;
		}
	}
	/* added OK */
	return result;
}

/*--------------------------------------------------------- */
