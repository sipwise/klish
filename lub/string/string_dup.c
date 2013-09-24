/*
 * string_dup.c
 */
#include <stdlib.h>
#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
char *lub_string_dup(const char *string)
{
	if (!string)
		return NULL;
	return strdup(string);
}

/*--------------------------------------------------------- */
char *lub_string_dupn(const char *string, unsigned int len)
{
	char *res = NULL;

	if (!string)
		return res;
	res = malloc(len + 1);
	strncpy(res, string, len);
	res[len] = '\0';

	return res;
}


/*--------------------------------------------------------- */
