/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_find.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation searches the specified "tree" for a "clientnode"
 * which matches the specified "key"
 *
 * tree - the "tree" instance to invoke this operation upon
 * key  - the "key" to search with
 *
 * RETURNS
 * "clientnode" instance or NULL if no node is found.
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:29:54 2004
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
 *    update to use new node,key comparison ordering
 * 28-Jan-2004		Graeme McKerrell	
 *   Initial version 
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
void *lub_bintree_find(lub_bintree_t * this, const void *clientkey)
{
	this->root = lub_bintree_splay(this, this->root, clientkey);

	if (NULL != this->root) {
		if (lub_bintree_compare(this, this->root, clientkey) == 0)
			return lub_bintree_getclientnode(this, this->root);
	}
	return NULL;
}

/*--------------------------------------------------------- */
