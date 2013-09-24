/*
 * argv__get_offset.c
 */
#include "private.h"

/*--------------------------------------------------------- */
size_t lub_argv__get_offset(const lub_argv_t * this, unsigned index)
{
	size_t result = 0;

	if (this->argc > index)
		result = this->argv[index].offset;

	return result;
}

/*--------------------------------------------------------- */
