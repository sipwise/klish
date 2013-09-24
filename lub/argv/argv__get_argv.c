/*
 * argv__get_argv.c
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
char *lub_argv__get_line(const lub_argv_t * this)
{
	int space = 0;
	const char *p;
	unsigned i;
	char *line = NULL;

	for (i = 0; i < this->argc; i++) {
		if (i != 0)
			lub_string_cat(&line, " ");
		space = 0;
		/* Search for spaces */
		for (p = this->argv[i].arg; *p; p++) {
			if (isspace(*p)) {
				space = 1;
				break;
			}
		}
		if (space)
			lub_string_cat(&line, "\"");
		lub_string_cat(&line, this->argv[i].arg);
		if (space)
			lub_string_cat(&line, "\"");
	}

	return line;
}

/*--------------------------------------------------------- */
char **lub_argv__get_argv(const lub_argv_t * this, const char *argv0)
{
	char **result = NULL;
	unsigned i;
	unsigned a = 0;

	if (argv0)
		a = 1;

	result = malloc(sizeof(char *) * (this->argc + 1 + a));

	if (argv0)
		result[0] = strdup(argv0);
	for (i = 0; i < this->argc; i++)
		result[i + a] = strdup(this->argv[i].arg);
	result[i + a] = NULL;

	return result;
}

/*--------------------------------------------------------- */
void lub_argv__free_argv(char **argv)
{
	unsigned i;

	if (!argv)
		return;

	for (i = 0; argv[i]; i++)
		free(argv[i]);
	free(argv);
}

/*--------------------------------------------------------- */
