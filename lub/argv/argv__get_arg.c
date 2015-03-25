/*
 * argv__get_arg.c
 */
#include "private.h"

/*--------------------------------------------------------- */
const char *lub_argv__get_arg(const lub_argv_t *this, unsigned int index)
{
	const char *result = NULL;

	if (!this)
		return NULL;
	if (this->argc > index)
		result = this->argv[index].arg;

	return result;
}

/*--------------------------------------------------------- */
