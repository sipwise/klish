/*
 * system_test.c
 */

#include <stdlib.h>

#include "private.h"

/*--------------------------------------------------------- */
bool_t lub_system_test(int argc, char **argv)
{
	return testcmd(argc, argv) ? BOOL_FALSE : BOOL_TRUE;
}

/*--------------------------------------------------------- */
bool_t lub_system_line_test(const char *line)
{
	bool_t res;
	lub_argv_t *argv;

	argv = lub_argv_new(line, 0);
	res = lub_system_argv_test(argv);
	lub_argv_delete(argv);

	return res;
}

/*--------------------------------------------------------- */
bool_t lub_system_argv_test(const lub_argv_t * argv)
{
	bool_t res;
	char **str_argv;
	int str_argc;

	/* Make args */
	str_argv = lub_argv__get_argv(argv, "");
	str_argc = lub_argv__get_count(argv) + 1;

	/* Test it */
	res = lub_system_test(str_argc, str_argv);
	lub_argv__free_argv(str_argv);

	return res;
}

/*--------------------------------------------------------- */
