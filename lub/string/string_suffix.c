/*
 * string_suffix.c
 */
#include "private.h"

#include "lub/ctype.h"
/*--------------------------------------------------------- */
const char *lub_string_suffix(const char *string)
{
	const char *p1, *p2;
	p1 = p2 = string;
	while (*p1) {
		if (lub_ctype_isspace(*p1)) {
			p2 = p1;
			p2++;
		}
		p1++;
	}
	return p2;
}

/*--------------------------------------------------------- */
