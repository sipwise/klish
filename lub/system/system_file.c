/*
 * system_file.c
 */

#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "lub/string.h"

/*-------------------------------------------------------- */
/* perform a simple tilde substitution for the home directory
 * defined in HOME
 */
char *lub_system_tilde_expand(const char *path)
{
	char *home_dir = getenv("HOME");
	char *result = NULL;
	char *tilde;

	while ((tilde = strchr(path, '~'))) {
		lub_string_catn(&result, path, tilde - path);
		lub_string_cat(&result, home_dir);
		path = tilde + 1;
	}
	lub_string_cat(&result, path);

	return result;
}
