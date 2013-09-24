/*
 * string_escape.c
 */
#include "private.h"

#include <stdlib.h>
#include <string.h>

const char *lub_string_esc_default = "`|$<>&()#;\\\"!";
const char *lub_string_esc_regex = "^$.*+[](){}";
const char *lub_string_esc_quoted = "\\\"";

/*--------------------------------------------------------- */
char *lub_string_ndecode(const char *string, unsigned int len)
{
	const char *s = string;
	char *res, *p;
	int esc = 0;

	if (!string)
		return NULL;

	/* Allocate enough memory for result */
	p = res = malloc(len + 1);

	while (*s && (s < (string +len))) {
		if (!esc) {
			if ('\\' == *s)
				esc = 1;
			else
				*p = *s;
		} else {
/*			switch (*s) {
			case 'r':
			case 'n':
				*p = '\n';
				break;
			case 't':
				*p = '\t';
				break;
			default:
				*p = *s;
				break;
			}
*/			*p = *s;
			esc = 0;
		}
		if (!esc)
			p++;
		s++;
	}
	*p = '\0';

	return res;
}

/*--------------------------------------------------------- */
inline char *lub_string_decode(const char *string)
{
	return lub_string_ndecode(string, strlen(string));
}

/*----------------------------------------------------------- */
/*
 * This needs to escape any dangerous characters within the command line
 * to prevent gaining access to the underlying system shell.
 */
char *lub_string_encode(const char *string, const char *escape_chars)
{
	char *result = NULL;
	const char *p;

	if (!escape_chars)
		return lub_string_dup(string);
	if (string && !(*string)) /* Empty string */
		return lub_string_dup(string);

	for (p = string; p && *p; p++) {
		/* find any special characters and prefix them with '\' */
		size_t len = strcspn(p, escape_chars);
		lub_string_catn(&result, p, len);
		p += len;
		if (*p) {
			lub_string_catn(&result, "\\", 1);
			lub_string_catn(&result, p, 1);
		} else {
			break;
		}
	}
	return result;
}

/*--------------------------------------------------------- */
