/*
 * string_cat.c
 */
#include "private.h"

#include <string.h>
#include <stdlib.h>
/*--------------------------------------------------------- */
void lub_string_catn(char **string, const char *text, size_t len)
{
	if (text) {
		char *q;
		size_t length, initlen, textlen = strlen(text);

		/* make sure the client cannot give us duff details */
		len = (len < textlen) ? len : textlen;

		/* remember the size of the original string */
		initlen = *string ? strlen(*string) : 0;

		/* account for '\0' */
		length = initlen + len + 1;

		/* allocate the memory for the result */
		q = realloc(*string, length);
		if (NULL != q) {
			*string = q;
			/* move to the end of the initial string */
			q += initlen;

			while (len--) {
				*q++ = *text++;
			}
			*q = '\0';
		}
	}
}

/*--------------------------------------------------------- */
