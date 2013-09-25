/*
 * sym_navy.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/argv.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <fcntl.h>

/*----------------------------------------------------------- */
/* Terminate the current shell session */
CLISH_PLUGIN_SYM(clish_close)
{
	clish_shell_t *this = clish_context__get_shell(clish_context);
	clish_shell__set_state(this, SHELL_STATE_CLOSING);
	return 0;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Whether the script continues after command, but not script, 
 errors depends on the value of the stop_on_error flag.
*/
static int clish_source_internal(clish_context_t *context,
	const char *fn, bool_t stop_on_error)
{
	int result = -1;
	const char *filename = fn;
	struct stat fileStat;

	/* the exception proves the rule... */
	clish_shell_t *this = clish_context__get_shell(context);

	/*
	 * Check file specified is not a directory 
	 */
	if ((0 == stat((char *)filename, &fileStat)) &&
		(!S_ISDIR(fileStat.st_mode))) {
		/*
		 * push this file onto the file stack associated with this
		 * session. This will be closed by clish_shell_pop_file() 
		 * when it is finished with.
		 */
		result = clish_shell_push_file(this, filename,
			stop_on_error);
	}

	return result ? -1 : 0;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Invoking a script in this way will cause the script to
 stop on the first error
*/
CLISH_PLUGIN_SYM(clish_source)
{
	clish_context_t *context = (clish_context_t *)clish_context;
	return (clish_source_internal(context, script, BOOL_TRUE));
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Invoking a script in this way will cause the script to
 continue after command, but not script, errors.
*/
CLISH_PLUGIN_SYM(clish_source_nostop)
{
	clish_context_t *context = (clish_context_t *)clish_context;
	return (clish_source_internal(context, script, BOOL_FALSE));
}

/*----------------------------------------------------------- */
/*
 Show the shell overview
*/
CLISH_PLUGIN_SYM(clish_overview)
{
	clish_shell_t *this = clish_context__get_shell(clish_context);
	tinyrl_t *tinyrl = clish_shell__get_tinyrl(this);
	tinyrl_printf(tinyrl, "%s\n", clish_shell__get_overview(this));
	return 0;
}

/*----------------------------------------------------------- */
CLISH_PLUGIN_SYM(clish_history)
{
	clish_shell_t *this = clish_context__get_shell(clish_context);
	tinyrl_t *tinyrl = clish_shell__get_tinyrl(this);
	tinyrl_history_t *history = tinyrl__get_history(tinyrl);
	tinyrl_history_iterator_t iter;
	const tinyrl_history_entry_t *entry;
	unsigned limit = 0;
	const char *arg = script;

	if (arg && ('\0' != *arg)) {
		limit = (unsigned)atoi(arg);
		if (0 == limit) {
			/* unlimit the history list */
			(void)tinyrl_history_unstifle(history);
		} else {
			/* limit the scope of the history list */
			tinyrl_history_stifle(history, limit);
		}
	}
	for (entry = tinyrl_history_getfirst(history, &iter);
		entry; entry = tinyrl_history_getnext(&iter)) {
		/* dump the details of this entry */
		tinyrl_printf(tinyrl,
			"%5d  %s\n",
			tinyrl_history_entry__get_index(entry),
			tinyrl_history_entry__get_line(entry));
	}
	return 0;
}

/*----------------------------------------------------------- */
/*
 * Find out the previous view in the stack and go to it
 */
CLISH_PLUGIN_SYM(clish_nested_up)
{
	clish_shell_t *this = clish_context__get_shell(clish_context);
	unsigned int depth;

	if (!this)
		return -1;
	/* If depth=0 than exit */
	if ((depth = clish_shell__get_depth(this)) == 0) {
		clish_shell__set_state(this, SHELL_STATE_CLOSING);
		return 0;
	}
	depth--;
	clish_shell__set_depth(this, depth);

	return 0;
}

/*----------------------------------------------------------- */
/*
 * Builtin: NOP function
 */
CLISH_PLUGIN_SYM(clish_nop)
{
	return 0;
}

/*----------------------------------------------------------- */
/*
 * Builtin: Set watchdog timeout. The "0" to turn watchdog off.
 */
CLISH_PLUGIN_SYM(clish_wdog)
{
	const char *arg = script;
	clish_shell_t *this = clish_context__get_shell(clish_context);

	/* Turn off watchdog if no args */
	if (!arg || ('\0' == *arg)) {
		clish_shell__set_wdog_timeout(this, 0);
		return 0;
	}

	clish_shell__set_wdog_timeout(this, (unsigned int)atoi(arg));

	return 0;
}

/*--------------------------------------------------------- */
/*
 * Get the ACTION context as a macros
 */
CLISH_PLUGIN_SYM(clish_macros)
{
	if (!script) /* Nothing to do */
		return 0;
	*out = lub_string_dup(script);
	return 0;
}

/*----------------------------------------------------------- */
