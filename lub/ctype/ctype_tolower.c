/*
 * ctype_tolower.c
 */
#include "lub/ctype.h"
#include <ctype.h>

/*--------------------------------------------------------- */
char lub_ctype_tolower(char c)
{
	unsigned char tmp = (unsigned char)c;
	return tolower(tmp);
}

/*--------------------------------------------------------- */
