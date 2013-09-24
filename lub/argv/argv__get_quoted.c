/*
 * argv__get_quoted.c
 */
#include "private.h"

/*--------------------------------------------------------- */
bool_t lub_argv__get_quoted(const lub_argv_t * this, unsigned index)
{
	bool_t result = BOOL_FALSE;

	if (this->argc > index)
		result = this->argv[index].quoted;

	return result;
}

/*--------------------------------------------------------- */
