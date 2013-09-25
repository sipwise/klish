/*
 * shell_misc.c
 */

#include <stdlib.h>
#include <assert.h>

#include "private.h"

/*--------------------------------------------------------- */
const char *clish_shell__get_overview(const clish_shell_t *this)
{
	assert(this);
	return this->overview;
}

/*--------------------------------------------------------- */
