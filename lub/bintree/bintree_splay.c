/*********************** -*- Mode: C -*- ***********************
 * File            : bintree_splay.c
 *---------------------------------------------------------------
 * Description
 * ===========
 *              An implementation of top-down splaying
 *                  D. Sleator <sleator@cs.cmu.edu>
 *  	                     March 1992
 *
 * "Splay trees", or "self-adjusting search trees" are a simple and
 * efficient data structure for storing an ordered set.  The data
 * structure consists of a binary tree, without parent pointers, and
 * no additional fields.  It allows searching, insertion, deletion,
 * deletemin, deletemax, splitting, joining, and many other
 * operations, all with amortized logarithmic performance.  Since the
 * trees adapt to the sequence of requests, their performance on real
 * access patterns is typically even better.  Splay trees are
 * described in a number of texts and papers [1,2,3,4,5].
 *
 * The code here is adapted from simple top-down splay, at the bottom
 * of page 669 of [3].  It can be obtained via anonymous ftp from
 * spade.pc.cs.cmu.edu in directory /usr/sleator/public.
 * 
 * The chief modification here is that the splay operation works even
 * if the item being splayed is not in the tree, and even if the tree
 * root of the tree is NULL.  So the line:
 *
 *                            t = splay(i, t);
 *
 * causes it to search for item with key i in the tree rooted at t.
 * If it's there, it is splayed to the root.  If it isn't there,
 * then the node put at the root is the last one before NULL that
 * would have been reached in a normal binary search for i.  (It's a
 * neighbor of i in the tree.)  This allows many other operations to
 * be easily implemented, as shown below.
 *
 * [1] "Fundamentals of data structures in C", Horowitz, Sahni,
 *      and Anderson-Freed, Computer Science Press, pp 542-547.
 * [2] "Data Structures and Their Algorithms", Lewis and Denenberg,
 *      Harper Collins, 1991, pp 243-251.
 * [3] "Self-adjusting Binary Search Trees" Sleator and Tarjan,
 *     JACM Volume 32, No 3, July 1985, pp 652-686.
 * [4] "Data Structure and Algorithm Analysis", Mark Weiss,
 *      Benjamin Cummins, 1992, pp 119-130.
 * [5] "Data Structures, Algorithms, and Performance", Derick Wood,
 *      Addison-Wesley, 1993, pp 367-375.
 *
 * The following code was written by Daniel Sleator, and is released
 * in the public domain.
 *
 *---------------------------------------------------------------
 * Author          : Graeme McKerrell
 * Created On      : Wed Jan 28 16:29:28 2004
 * Status          : TESTED
 *---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Renamed to the "lub_" namespace
 * 5-May-2004		Graeme McKerrell	
 *    updates following review
 * 2-Mar-2004		Graeme McKerrell	
 *    Fixed to account for comparision logic being (node - key)
 *    original algorithm expected (key - node)
 * 9-Feb-2004		Graeme McKerrell	
 *    updated to use new node,key comparison ordering
 * 28-Jan-2004		Graeme McKerrell	
 *    Adapted from original public domain code
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 **************************************************************** */
#include "private.h"

/*--------------------------------------------------------- */
lub_bintree_node_t *lub_bintree_splay(const lub_bintree_t * this,
				      lub_bintree_node_t * t, const void *key)
{
	/* Simple top down splay, not requiring "key" to be in the tree t.  */
	/* What it does is described above.                                 */
	lub_bintree_node_t N, *leftTreeMax, *rightTreeMin, *y;
	int comp;

	if (t == NULL)
		return t;
	N.left = N.right = NULL;
	leftTreeMax = rightTreeMin = &N;

	for (;;) {
		comp = lub_bintree_compare(this, t, key);
		if (comp > 0) {
			if (t->left == NULL)
				break;
			if (lub_bintree_compare(this, t->left, key) > 0) {
				y = t->left;	/* rotate right */
				t->left = y->right;
				y->right = t;
				t = y;
				if (t->left == NULL)
					break;
			}
			rightTreeMin->left = t;	/* link right */
			rightTreeMin = t;
			t = t->left;
		} else if (comp < 0) {
			if (t->right == NULL)
				break;
			if (lub_bintree_compare(this, t->right, key) < 0) {
				y = t->right;	/* rotate left */
				t->right = y->left;
				y->left = t;
				t = y;
				if (t->right == NULL)
					break;
			}
			leftTreeMax->right = t;	/* link left */
			leftTreeMax = t;
			t = t->right;
		} else {
			break;
		}
	}
	leftTreeMax->right = t->left;	/* assemble */
	rightTreeMin->left = t->right;
	t->left = N.right;
	t->right = N.left;

	/* return the new root */
	return t;
}

/*--------------------------------------------------------- */
