/*
 * ctype_isdigit.c
 */
#include "lub/ctype.h"
#include <ctype.h>

/*--------------------------------------------------------- */
bool_t lub_ctype_isdigit(char c)
{
	unsigned char tmp = (unsigned char)c;
	return isdigit(tmp) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
