/**
\ingroup lub
\defgroup lub_bintree bintree
 @{

\brief This interface provides a facility which enables a client to 
 order and access a set of arbitary data in a binary "tree"
 
 Each "tree" is defined by a number of "clientnodes" which are
 ordered according to a client defined "key".

 A "clientkey" is a client specific entity which can be compared
 with a "clientnode" to determine which is the "greatest". In order
 to do this the client needs provide a comparison function for
 comparing a "clientnode" with a "clientkey", and a function to
 convert a "clientnode" into a "clientkey".

 The client is responsible for providing each "clientnode" in a
 tree. This will typically contain some client specific data, but
 will also need to contain a bintree "node" which is used to
 structurally relate each node to one another in the tree. A
 specific "node" may only belong to one tree at a time, but an
 individual "clientnode" may contain multiple of these if necessary.

\par Implementation
The implementation of this interface uses a top-down splaying algorithm.

\par 
"Splay trees", or "self-adjusting search trees" are a simple and
efficient data structure for storing an ordered set.  The data
structure consists of a binary tree, without parent pointers, and
no additional fields.  It allows searching, insertion, deletion,
deletemin, deletemax, splitting, joining, and many other
operations, all with amortized logarithmic performance.  Since the
trees adapt to the sequence of requests, their performance on real
access patterns is typically even better.  Splay trees are
described in a number of texts and papers [1,2,3,4,5].

\par
The code here is adapted from simple top-down splay, at the bottom
of page 669 of [3].  It can be obtained via anonymous ftp from
spade.pc.cs.cmu.edu in directory /usr/sleator/public.

\par
The chief modification here is that the splay operation works even
if the item being splayed is not in the tree, and even if the tree
root of the tree is NULL.  So the line:

\par
                           t = splay(i, t);

\par
causes it to search for item with key i in the tree rooted at t.
If it's there, it is splayed to the root.  If it isn't there,
then the node put at the root is the last one before NULL that
would have been reached in a normal binary search for i.  (It's a
neighbor of i in the tree.)  This allows many other operations to
be easily implemented.

\par
[1] "Fundamentals of data structures in C", Horowitz, Sahni,
     and Anderson-Freed, Computer Science Press, pp 542-547.

\par
[2] "Data Structures and Their Algorithms", Lewis and Denenberg,
     Harper Collins, 1991, pp 243-251.

\par
[3] "Self-adjusting Binary Search Trees" Sleator and Tarjan,
    JACM Volume 32, No 3, July 1985, pp 652-686.

\par
[4] "Data Structure and Algorithm Analysis", Mark Weiss,
    Benjamin Cummins, 1992, pp 119-130.

\par
[5] "Data Structures, Algorithms, and Performance", Derick Wood,
     Addison-Wesley, 1993, pp 367-375.

\par
The splay function is based on one written by Daniel Sleator, which is released
in the public domain.
 
\author  Graeme McKerrell
\date    Created On      : Fri Jan 23 12:50:18 2004
\version TESTED
 */
/*---------------------------------------------------------------
 HISTORY
 7-Dec-2004		Graeme McKerrell	
   Updated to use the "lub" prefix
27-Feb-2004		Graeme McKerrell	
   updated to simplify node initialisation
9-Feb-2004		Graeme McKerrell	
   updated to make the comparision function compare a "clientnode" and
   "key"
   Updated getkey() function to fill out a provides "key" from a "clientnode"
23-Jan-2004		Graeme McKerrell	
   Initial version
--------------------------------------------------------------
Copyright (C) 2004 3Com Corporation. All Rights Reserved.
**************************************************************** */
#ifndef _lub_bintree_h
#define _lub_bintree_h
#include <stddef.h>

/****************************************************************
 * TYPE DEFINITIONS
 **************************************************************** */
/**
 * This type represents a bintree "node".
 * Typically the client will have a "clientnode" structure which
 * contains it's data. A bintree "node" is made one of the data
 * elements of that structure. When the tree is initialised the
 * client provides the offset into the structure of the
 * "node" which is to be used for that tree.
 */
typedef struct lub_bintree_node_s lub_bintree_node_t;
/** 
 * CLIENTS MUSTN'T TOUCH THE CONTENTS OF THIS STRUCTURE
 */
struct lub_bintree_node_s {
	/** internal */ lub_bintree_node_t *left;
	/** internal */ lub_bintree_node_t *right;
};

/**
 * This type defines a callback function which will compare two "keys"
 * with each other
 *
 * \param clientnode 	the client node to compare
 * \param clientkey 	the key to compare with a node
 *
 * \return
 *     <0 if clientnode  < clientkey;
 *      0 if clientnode == clientkey;
 *     >0 if clientnode  > clientkey
 */
typedef int
lub_bintree_compare_fn(const void *clientnode, const void *clientkey);
/**
 * This is used to size the key storage area for an opaque key.
 * If any client requires a greater storage size then this will need to
 * be increased.
 */
#define lub_bintree_MAX_KEY_STORAGE (200)
/**
 * This is used to declare an opaque key structure
 * Typically a client would declare their own non-opaque structure
 * which they would fill out appropriately
 */
typedef struct lub_bintree_key_s lub_bintree_key_t;
/** 
 * CLIENTS MUSTN'T TOUCH THE CONTENTS OF THIS STRUCTURE
 */
struct lub_bintree_key_s {
    /** internal */ char storage[lub_bintree_MAX_KEY_STORAGE];
    /** internal */ int magic;
};

/**
 * This type defines a callback function which will convert a client's "node"
 * into a search "key"
 * 
 * \param clientnode 	the node from which to derive a key
 * \param key        	a reference to the key to fill out
 * 
 * \return
 * A "key" which corresponds the "node" in this view
 */
typedef void
lub_bintree_getkey_fn(const void *clientnode, lub_bintree_key_t * key);

/**
 * This type represents an binary tree instance
 */
typedef struct lub_bintree_s lub_bintree_t;
/** 
 * CLIENTS MUSTN'T TOUCH THE CONTENTS OF THIS STRUCTURE
 */
struct lub_bintree_s {
	/** internal */ lub_bintree_node_t *root;
	/** internal */ size_t node_offset;
	/** internal */ lub_bintree_compare_fn *compareFn;
	/** internal */ lub_bintree_getkey_fn *getkeyFn;
};

/**
 * This is used to perform iterations of a tree
 */
typedef struct lub_bintree_iterator_s lub_bintree_iterator_t;
/** 
 * CLIENTS MUSTN'T TOUCH THE CONTENTS OF THIS STRUCTURE
 */
struct lub_bintree_iterator_s {
	/** internal */ lub_bintree_t *tree;
    /** internal */ lub_bintree_key_t key;
};

/****************************************************************
 * BINTREE OPERATIONS
 **************************************************************** */
/**
 * This operation initialises an instance of a binary tree.
 *
 * \pre none
 *
 * \post The tree is ready to have client nodes inserted.
 */
extern void lub_bintree_init(
		/** 
		* the "tree" instance to initialise 
		*/
				    lub_bintree_t * tree,
		/** 
		* the offset of the bintree "node" structure within the
		* "clientnode" structure. This is typically passed 
		* using the offsetof() macro.
		*/
				    size_t node_offset,
		/**
		* a comparison function for comparing a "clientnode"
		* with a "clientkey"
		*/
				    lub_bintree_compare_fn compareFn,
		/**
		* a function which will fill out a "key" from a clientnode
		*/
				    lub_bintree_getkey_fn getkeyFn);

/**
 * This operation is called to initialise a "clientnode" ready for
 * insertion into a tree. This is only required once after the memory
 * for a node has been allocated.
 *
 * \pre none
 *
 * \post The node is ready to be inserted into a tree.
 */
extern void lub_bintree_node_init(
			/** 
			 * the bintree node to initialise
			  */
					 lub_bintree_node_t * node);

/*****************************************
 * NODE MANIPULATION OPERATIONS
 ***************************************** */
/**
 * This operation adds a client node to the specified tree.
 *
 * \pre The tree must be initialised
 * \pre The clientnode must be initialised
 * 
 * \return
 * 0 if the "clientnode" is added correctly to the tree.
 * If another "clientnode" already exists in the tree with the same key, then
 * -1 is returned, and the tree remains unchanged.
 *
 * \post If the bintree "node" is already part of a tree, then an
 * assert will fire.
 */
extern int lub_bintree_insert(
			/**
			 * the "tree" instance to invoke this operation upon
			 */
				     lub_bintree_t * tree,
			/** 
			 * a pointer to a client node. NB the tree can find the
			 * necessary lub_BintreeNodeT from it's stored offset.
			 */
				     void *clientnode);

/**
 * This operation removes a "clientnode" from the specified "tree"
 *
 * \pre The tree must be initialised
 * \pre The clientnode must be initialised
*
 * \post The "clientnode" will no longer be part of the specified tree, and will be
 * made available for re-insertion
 * \post If the clientnode is not present in the specified tree, then an
 *  assert will fire.
 */
extern void lub_bintree_remove(
			/** 
			* the "tree" instance to invoke this operation upon
			*/
				      lub_bintree_t * tree,
			/**
			* the node to remove
			*/
				      void *clientnode);

/*****************************************
 * NODE RETRIEVAL OPERATIONS
 ***************************************** */
/**
 * This operation returns the first "clientnode" present in the specified "tree"
 *
 * \pre The tree must be initialised
 *
 * \return
 * "clientnode" instance or NULL if no nodes are present in this tree.
 */
extern void *lub_bintree_findfirst(
		/** 
		 * the "tree" instance to invoke this operation upon
		 */
					  lub_bintree_t * tree);

/**
 * This operation returns the last "clientnode" present in the specified "tree"
 *
 * \pre The tree must be initialised
 *
 * \return
 * "clientnode" instance or NULL if no nodes are present in this tree.
 */
extern void *lub_bintree_findlast(
		/** 
		* the "tree" instance to invoke this operation upon
		*/
					 lub_bintree_t * tree);

/**
 * This operation searches the specified "tree" for a "clientnode" which matches the
 * specified "key"
 *
 * \pre The tree must be initialised
 *
 * \return
 * "clientnode" instance or NULL if no node is found.
 */
extern void *lub_bintree_find(
		/** 
		 * the "tree" instance to invoke this operation upon
		 */
				     lub_bintree_t * tree,
		/** 
		  * the "key" to search with
		  */
				     const void *key);

/**
 * This operation searches the specified "tree" for a "clientnode" which is
 * the one which logically follows the specified "key"
 *
 * A "clientnode" with the specified "key" doesn't need to be in the tree.
 *
 * \pre The tree must be initialised
 *
 * \return
 * "clientnode" instance or NULL if no node is found.
 */
extern void *lub_bintree_findnext(
		/** 
		 * the "tree" instance to invoke this operation upon
		 */
					 lub_bintree_t * tree,
		/** 
		  * the "key" to search with
		  */
					 const void *key);

/**
 * This operation searches the specified "tree" for a "clientnode" which is
 * the one which logically preceeds the specified "key"
 *
 * A "clientnode" with the specified "key" doesn't need to be in the tree.
 *
 * \pre The tree must be initialised
 *
 * \return
 * "clientnode" instance or NULL if no node is found.
 */
extern void *lub_bintree_findprevious(
		/** 
		 * the "tree" instance to invoke this operation upon
		 */
					     lub_bintree_t * tree,
		/** 
		  * the "key" to search with
		  */
					     const void *key);

/*****************************************
 * ITERATION OPERATIONS
 ***************************************** */
/**
 * This operation initialises an iterator. This can then be
 * subsequently used for iterating through a tree. This will work
 * even if the "clientnode" which defined the current iterator has been
 * removed before the next iterator operation.
 *
 * \pre The tree must be initialised
 * \pre The clientnode must be initialised and valid at the time of this call
 *
 * \post The interator instance will be updated to reference the position in the tree for the clientnode.
 */
extern void lub_bintree_iterator_init(
			/** 
			* the iterator instance to initialise
			*/
					     lub_bintree_iterator_t * iter,
			/** 
			* the tree to associate with this iterator
			*/
					     lub_bintree_t * tree,
			/** 
			 * the starting point for the iteration
			 */
					     const void *clientnode);

/**
 * This operation returns the next "clientnode" in an iteration.
 *
 * \pre The interator instance must have been initialised
 *
 * \return
 * "clientnode" instance or NULL if the iteration has reached the end of the
 * tree.
 *
 * \post The interator instance will be updated to reference the position in the tree for the returned value.
 */
extern void *lub_bintree_iterator_next(
		/** 
		 * the iterator instance to invoke this operation upon.
		 */
					      lub_bintree_iterator_t * iter);

/**
 * This operation returns the previous "clientnode" in an iteration.
 *
 * \pre The interator instance must have been initialised
 *
 * \return
 * "clientnode" instance or NULL if the iteration has reached the beginning
 * of the tree.
 *
 * \post The interator instance will be updated to reference the position in the tree for the returned value.
 */
extern void *lub_bintree_iterator_previous(
		/** 
		 * the iterator instance to invoke this operation upon.
		 */
						  lub_bintree_iterator_t *
						  iter);
/**
 * This operation dumps the node list of the specified tree to stdout
 *
 * \pre The tree must be initialised
 *
 * \post The structure of the tree will be unaltered.
 */
extern void lub_bintree_dump(
	/** 
	 * the "tree" instance to invoke this operation upon
	 */
				    lub_bintree_t * tree);

#endif				/* _lub_bintree_h */
/** @} */
