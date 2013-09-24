/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_findlast.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation returns the last "clientnode" present in the
 * specified "tree"
 *
 * tree - the "tree" instance to invoke this operation upon
 *
 * RETURNS
 * "clientnode" instance or NULL if no nodes are present in this view.
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:17:17 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 2-Mar-2004		Graeme McKerrell	
 *    fixed comparison logic
 * 9-Feb-2004		Graeme McKerrell	
 *    update to use new getkey prototype, removed the now spurious
 *    nullgetkey() function
 * 28-Jan-2004		Graeme McKerrell	
 *   Initial version 
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/* forward declare these functions */
static lub_bintree_compare_fn compareright;

/*--------------------------------------------------------- */
void *lub_bintree_findlast(lub_bintree_t * this)
{
	lub_bintree_compare_fn *client_compare = this->compareFn;

	/*
	 * put dummy functions in place
	 * This will make the search faster and direct it to the right most
	 * node
	 */
	this->compareFn = compareright;

	/*
	 * the key doesn't matter here cos we've cobbled the compare function
	 */
	this->root = lub_bintree_splay(this, this->root, NULL);

	/* restore the client functions */
	this->compareFn = client_compare;

	if (NULL == this->root)
		return NULL;
	else
		return lub_bintree_getclientnode(this, this->root);
}

/*--------------------------------------------------------- */
/*
 * This comparison operation always returns -1 hence will force a
 * search to the left most node.
 *
 * key1 - first node to compare (not used)
 * key2 - second node to compare (not used)
 */
static int compareright(const void *clientnode, const void *clientkey)
{
	clientnode = clientnode;
	clientkey = clientkey;
	return -1;
}

/*--------------------------------------------------------- */
