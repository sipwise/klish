/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_node_init.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation is called to initialise a "clientnode" ready for
 * insertion into a tree. This is only required once after the memory
 * for a node has been allocated.
 *
 * tree       - the tree instance to invoke this operation upon
 * clientnode - the node to initialise.
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 09:57:06 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
  * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 27-Feb-2004		Graeme McKerrell	
 *    Updated to simplify the initialisation of nodes
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
void lub_bintree_node_init(lub_bintree_node_t * node)
{
	assert(node);
	if (node) {
		node->left = NULL;
		node->right = NULL;
	}
}

/*--------------------------------------------------------- */
