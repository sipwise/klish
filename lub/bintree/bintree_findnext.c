/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_findnext.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation searches the specified "tree" for a "clientnode" which is
 * the one which logically follows the specified "key"
 *
 * A "clientnode" with the specified "key" doesn't need to be in the tree.
 *
 * tree - the "tree" instance to invoke this operation upon
 * key  - the "key" to search with
 *
 * RETURNS
 * "clientnode" instance or NULL if no node is found.
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:32:28 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 27-Feb-2004		Graeme McKerrell	
 *    Fixed to account for empty tree
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use new node,key comparison ordering
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
void *lub_bintree_findnext(lub_bintree_t * this, const void *clientkey)
{
	lub_bintree_node_t *t = this->root;
	int comp;

	/*
	 * have a look for a direct match
	 */
	t = this->root = lub_bintree_splay(this, t, clientkey);

	if (NULL != t) {
		/* now look at what we have got */
		comp = lub_bintree_compare(this, t, clientkey);
		if (comp <= 0) {
			/*
			 * time to fiddle with the right hand side of the tree
			 * we need the closest node from the right hand side
			 */
			t = t->right =
			    lub_bintree_splay(this, t->right, clientkey);
		}
	}
	if (NULL == t) {
		return NULL;
	} else {
		return lub_bintree_getclientnode(this, t);
	}
}

/*--------------------------------------------------------- */
