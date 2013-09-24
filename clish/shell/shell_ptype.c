/*
 * shell_find_create_ptype.c
 */

#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
clish_ptype_t *clish_shell_find_ptype(clish_shell_t *this,
	const char *name)
{
	return lub_bintree_find(&this->ptype_tree, name);
}

/*--------------------------------------------------------- */
clish_ptype_t *clish_shell_find_create_ptype(clish_shell_t * this,
	const char *name, const char *text, const char *pattern,
	clish_ptype_method_e method, clish_ptype_preprocess_e preprocess)
{
	clish_ptype_t *ptype = lub_bintree_find(&this->ptype_tree, name);

	if (!ptype) {
		/* create a ptype */
		ptype = clish_ptype_new(name, text, pattern,
			method, preprocess);
		assert(ptype);
		clish_shell_insert_ptype(this, ptype);
	} else {
		if (pattern) {
			/* set the pattern */
			clish_ptype__set_pattern(ptype, pattern, method);
			/* set the preprocess */
			clish_ptype__set_preprocess(ptype, preprocess);
		}
		/* set the help text */
		if (text)
			clish_ptype__set_text(ptype, text);
	}

	return ptype;
}

/*--------------------------------------------------------- */
void clish_shell_insert_ptype(clish_shell_t * this, clish_ptype_t * ptype)
{
	(void)lub_bintree_insert(&this->ptype_tree, ptype);
}

/*--------------------------------------------------------- */
