/*********************** -*- Mode: C -*- ***********************
 * File            : private.h
 *---------------------------------------------------------------
 * Description
 * ===========
 * This defines the private interface used internally by this component
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 08:45:01 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 9-Feb-2004		Graeme McKerrell	
 *    modified compare MACRO
 * 28-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "lub/bintree.h"

/*************************************************************
 * PRIVATE OPERATIONS
 ************************************************************* */
/*------------------------------------------------------------ */
/* This is the operation which performs a top-down splay. It is
 * the core workhorse for this tree implementation.
 *
 * tree - the instance to invoke this operation upon
 * t    - the root node to splay to.
 * key  - the value with which to splay
 */
extern lub_bintree_node_t *lub_bintree_splay(const lub_bintree_t * tree,
					     lub_bintree_node_t * t,
					     const void *key);
/*------------------------------------------------------------ */
/* This operation converts a "node" into a "clientnode"
 * subtracting the offset gives the base pointer to the node
 *
 * this - the tree to invoke this operation upon
 * node - the node to convert
 */
#define lub_bintree_getclientnode(this,node)\
(void *)(((char*)node) - this->node_offset)
/*------------------------------------------------------------ */
/* This operation converts a "clientnode" into a "node"
 * adding the offset gives the base pointer to the node
 *
 * this       - the tree to invoke this operation upon
 * clientnode - the clientnode to convert
 */
#define lub_bintree_getnode(this,clientnode)\
(lub_bintree_node_t *)(((char*)clientnode) + this->node_offset)	/*lint -e826 */
/*------------------------------------------------------------ */
/* This operation compares a key with a "node"
 * it returns
 * <0 if key <  node
 *  0 if key == node
 * >0 if key >  node
 *
 * this - the tree to invoke this operation upon
 * node - the "node" to compare
 * key  - the key to compare
 */
#define lub_bintree_compare(this,node,key)\
(this)->compareFn(lub_bintree_getclientnode(this,node),key)
/*------------------------------------------------------------ */
