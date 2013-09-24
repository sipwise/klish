/*
 * string_cat.c
 */
#include "private.h"

#include <string.h>
/*--------------------------------------------------------- */
void lub_string_cat(char **string, const char *text)
{
	size_t len = text ? strlen(text) : 0;
	lub_string_catn(string, text, len);
}

/*--------------------------------------------------------- */
