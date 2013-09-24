/*
 * ctype_isspace.c
 */
#include "lub/ctype.h"
#include <ctype.h>

/*--------------------------------------------------------- */
bool_t lub_ctype_isspace(char c)
{
	unsigned char tmp = (unsigned char)c;
	return isspace(tmp) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
