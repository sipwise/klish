/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_iterator_previous.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation returns the previous "clientnode" in an iteration.
 *
 * iter - the iterator instance to invoke this operation upon.
 *
 * RETURNS
 * "clientnode" instance or NULL if the iteration has reached the beginning
 * of the tree.
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:35:19 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use new key structure
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
void *lub_bintree_iterator_previous(lub_bintree_iterator_t * this)
{
	void *clientnode = lub_bintree_findprevious(this->tree, &this->key);

	/* make sure that next time we've move onward */
	lub_bintree_iterator_init(this, this->tree, clientnode);

	return clientnode;
}

/*--------------------------------------------------------- */
