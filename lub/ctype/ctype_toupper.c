/*
 * ctype_toupper.c
 */
#include "lub/ctype.h"
#include <ctype.h>

/*--------------------------------------------------------- */
char lub_ctype_toupper(char c)
{
	unsigned char tmp = (unsigned char)c;
	return toupper(tmp);
}

/*--------------------------------------------------------- */
