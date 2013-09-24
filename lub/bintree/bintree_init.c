/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_init.c
 *---------------------------------------------------------------
 * Description
 * ===========
 * This operations initialise an instance of a binary tree at runtime.
 *
 * this       - the "tree" instance to initialise
 *
 * offset     - the offset of the node structure within the clients
 *              structure. This is typically passed using the offsetof() macro.
 *
 * compareFn  - a comparison function for comparing a "clientnode"
 *              with a "clientkey"
 *
 * getkeyFn   - a function which will fill out a "key" from a clientnode
 *
 * RETURNS
 * none
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 09:54:37 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 9-Feb-2004		Graeme McKerrell	
 *    update to remove spurious key_storage parameter
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
void
lub_bintree_init(lub_bintree_t * this,
		 size_t node_offset,
		 lub_bintree_compare_fn compareFn,
		 lub_bintree_getkey_fn getkeyFn)
{
	this->root = NULL;
	this->node_offset = node_offset;
	this->compareFn = compareFn;
	this->getkeyFn = getkeyFn;
}

/*--------------------------------------------------------- */
