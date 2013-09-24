/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_iterator_init.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operation initialises an iterator. This can then be
 * subsequently used for iterating through a tree. This will work
 * even if the "clientnode" which defined the current iterator has been
 * removed before the next iterator operation.
 *
 * iter       - the iterator instance to initialise
 * tree       - the tree to associate with this iterator
 * clientnode - the starting point for the iteration
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 10:33:42 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 3-Nov-2004		Graeme McKerrell	
 *    Added key bounds checking code
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use new getkey prototype
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"
#include <assert.h>

#define MAGIC_NUMBER 0x12345678
/*--------------------------------------------------------- */
void
lub_bintree_iterator_init(lub_bintree_iterator_t * this,
			  lub_bintree_t * tree, const void *clientnode)
{
	if (clientnode != NULL) {
		this->tree = tree;
		this->key.magic = MAGIC_NUMBER;
		/* fill out the iterator's key */
		this->tree->getkeyFn(clientnode, &this->key);
		/*
		 * this assert will fire if the client tries to store more than 
		 * the current storage permits
		 */
		assert(this->key.magic == MAGIC_NUMBER);
	}
}

/*--------------------------------------------------------- */
