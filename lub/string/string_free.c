/*
 * string_free.c
 */
#include "private.h"

#include <stdlib.h>

/*--------------------------------------------------------- */
void lub_string_free(char *ptr)
{
	free(ptr);
}

/*--------------------------------------------------------- */
