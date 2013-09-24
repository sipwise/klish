/*
 * shell_execute.c
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

/*
 * These are the internal commands for this framework.
 */
static clish_shell_builtin_fn_t
    clish_close,
    clish_overview,
    clish_source,
    clish_source_nostop,
    clish_history,
    clish_nested_up,
    clish_nop,
    clish_wdog,
    clish_macros;

static clish_shell_builtin_t clish_cmd_list[] = {
	{"clish_close", clish_close},
	{"clish_overview", clish_overview},
	{"clish_source", clish_source},
	{"clish_source_nostop", clish_source_nostop},
	{"clish_history", clish_history},
	{"clish_nested_up", clish_nested_up},
	{"clish_nop", clish_nop},
	{"clish_wdog", clish_wdog},
	{"clish_macros", clish_macros},
	{NULL, NULL}
};

/*----------------------------------------------------------- */
/* Terminate the current shell session */
static int clish_close(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *)context->shell;

	argv = argv; /* not used */
	this->state = SHELL_STATE_CLOSING;

	return 0;
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Whether the script continues after command, but not script, 
 errors depends on the value of the stop_on_error flag.
*/
static int clish_source_internal(clish_context_t *context,
	const lub_argv_t * argv, bool_t stop_on_error)
{
	int result = -1;
	const char *filename = lub_argv__get_arg(argv, 0);
	struct stat fileStat;

	/* the exception proves the rule... */
	clish_shell_t *this = (clish_shell_t *)context->shell;

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
static int clish_source(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	return (clish_source_internal(context, argv, BOOL_TRUE));
}

/*----------------------------------------------------------- */
/*
 Open a file and interpret it as a script in the context of a new
 thread. Invoking a script in this way will cause the script to
 continue after command, but not script, errors.
*/
static int clish_source_nostop(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	return (clish_source_internal(context, argv, BOOL_FALSE));
}

/*----------------------------------------------------------- */
/*
 Show the shell overview
*/
static int clish_overview(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	clish_shell_t *this = context->shell;
	argv = argv; /* not used */

	tinyrl_printf(this->tinyrl, "%s\n", context->shell->overview);

	return 0;
}

/*----------------------------------------------------------- */
static int clish_history(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	clish_shell_t *this = context->shell;
	tinyrl_history_t *history = tinyrl__get_history(this->tinyrl);
	tinyrl_history_iterator_t iter;
	const tinyrl_history_entry_t *entry;
	unsigned limit = 0;
	const char *arg = lub_argv__get_arg(argv, 0);

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
		tinyrl_printf(this->tinyrl,
			"%5d  %s\n",
			tinyrl_history_entry__get_index(entry),
			tinyrl_history_entry__get_line(entry));
	}
	return 0;
}

/*----------------------------------------------------------- */
/*
 * Searches for a builtin command to execute
 */
static clish_shell_builtin_fn_t *find_builtin_callback(const
	clish_shell_builtin_t * cmd_list, const char *name)
{
	const clish_shell_builtin_t *result;

	/* search a list of commands */
	for (result = cmd_list; result && result->name; result++) {
		if (0 == strcmp(name, result->name))
			break;
	}
	return (result && result->name) ? result->callback : NULL;
}

/*----------------------------------------------------------- */
void clish_shell_cleanup_script(void *script)
{
	/* simply release the memory */
	lub_string_free(script);
}

/*-------------------------------------------------------- */
static int clish_shell_lock(const char *lock_path)
{
	int i;
	int res;
	int lock_fd = -1;
	struct flock lock;

	if (!lock_path)
		return -1;
	lock_fd = open(lock_path, O_WRONLY | O_CREAT, 00644);
	if (-1 == lock_fd) {
		fprintf(stderr, "Can't open lockfile %s.\n", lock_path);
		return -1;
	}
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	for (i = 0; i < CLISH_LOCK_WAIT; i++) {
		res = fcntl(lock_fd, F_SETLK, &lock);
		if (res != -1)
			break;
		if (EINTR == errno)
			continue;
		if ((EAGAIN == errno) || (EACCES == errno)) {
			if (0 == i)
				fprintf(stderr,
					"Try to get lock. Please wait...\n");
			sleep(1);
			continue;
		}
		break;
	}
	if (res == -1) {
		fprintf(stderr, "Can't get lock.\n");
		close(lock_fd);
		return -1;
	}
	return lock_fd;
}

/*-------------------------------------------------------- */
static void clish_shell_unlock(int lock_fd)
{
	struct flock lock;

	if (lock_fd == -1)
		return;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	fcntl(lock_fd, F_SETLK, &lock);
	close(lock_fd);
}

/*----------------------------------------------------------- */
int clish_shell_execute(clish_context_t *context, char **out)
{
	clish_shell_t *this = context->shell;
	const clish_command_t *cmd = context->cmd;
	clish_action_t *action;
	int result = 0;
	char *lock_path = clish_shell__get_lockfile(this);
	int lock_fd = -1;
	sigset_t old_sigs;
	struct sigaction old_sigint, old_sigquit, old_sighup;
	clish_view_t *cur_view = clish_shell__get_view(this);
	unsigned int saved_wdog_timeout = this->wdog_timeout;

	assert(cmd);
	action = clish_command__get_action(cmd);

	/* Pre-change view if the command is from another depth/view */
	{
		clish_view_restore_t restore = clish_command__get_restore(cmd);
		if ((CLISH_RESTORE_VIEW == restore) &&
			(clish_command__get_pview(cmd) != cur_view)) {
			clish_view_t *view = clish_command__get_pview(cmd);
			clish_shell__set_pwd(this, NULL, view, NULL, context);
		} else if ((CLISH_RESTORE_DEPTH == restore) &&
			(clish_command__get_depth(cmd) < this->depth)) {
			this->depth = clish_command__get_depth(cmd);
		}
	}

	/* Lock the lockfile */
	if (lock_path && clish_command__get_lock(cmd)) {
		lock_fd = clish_shell_lock(lock_path);
		if (-1 == lock_fd) {
			result = -1;
			goto error; /* Can't set lock */
		}
	}

	/* Ignore and block SIGINT, SIGQUIT, SIGHUP */
	if (!clish_command__get_interrupt(cmd)) {
		struct sigaction sa;
		sigset_t sigs;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = SIG_IGN;
		sigaction(SIGINT, &sa, &old_sigint);
		sigaction(SIGQUIT, &sa, &old_sigquit);
		sigaction(SIGHUP, &sa, &old_sighup);
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGINT);
		sigaddset(&sigs, SIGQUIT);
		sigaddset(&sigs, SIGHUP);
		sigprocmask(SIG_BLOCK, &sigs, &old_sigs);
	}

	/* Execute ACTION */
	result = clish_shell_exec_action(action, context, out);

	/* Restore SIGINT, SIGQUIT, SIGHUP */
	if (!clish_command__get_interrupt(cmd)) {
		sigprocmask(SIG_SETMASK, &old_sigs, NULL);
		/* Is the signals delivery guaranteed here (before
		   sigaction restore) for previously blocked and
		   pending signals? The simple test is working well.
		   I don't want to use sigtimedwait() function bacause
		   it needs a realtime extensions. The sigpending() with
		   the sleep() is not nice too. Report bug if clish will
		   get the SIGINT after non-interruptable action.
		*/
		sigaction(SIGINT, &old_sigint, NULL);
		sigaction(SIGQUIT, &old_sigquit, NULL);
		sigaction(SIGHUP, &old_sighup, NULL);
	}

	/* Call config callback */
	if (!result && this->client_hooks->config_fn)
		this->client_hooks->config_fn(context);

	/* Call logging callback */
	if (clish_shell__get_log(this) && this->client_hooks->log_fn) {
		char *full_line = clish_shell__get_full_line(context);
		this->client_hooks->log_fn(context, full_line, result);
		lub_string_free(full_line);
	}

	/* Unlock the lockfile */
	if (lock_fd != -1)
		clish_shell_unlock(lock_fd);

	/* Move into the new view */
	if (!result) {
		clish_view_t *view = NULL;
		const char *view_str = clish_command__get_view(cmd);
		if (view_str) {
			char *view_exp = NULL;
			view_exp = clish_shell_expand(view_str,
				SHELL_VAR_NONE, context);
			view = clish_shell_find_view(this, view_exp);
			if (!view)
				fprintf(stderr, "System error: Can't "
					"change view to %s\n", view_exp);
			lub_string_free(view_exp);
		}
		/* Save the PWD */
		if (view) {
			char *line = clish_shell__get_line(context);
			clish_shell__set_pwd(this, line, view,
				clish_command__get_viewid(cmd), context);
			lub_string_free(line);
		}
	}

	/* Set appropriate timeout. Workaround: Don't turn on  watchdog
	on the "set watchdog <timeout>" command itself. */
	if (this->wdog_timeout && saved_wdog_timeout) {
		tinyrl__set_timeout(this->tinyrl, this->wdog_timeout);
		this->wdog_active = BOOL_TRUE;
		fprintf(stderr, "Warning: The watchdog is active. Timeout is %u "
			"seconds.\nWarning: Press any key to stop watchdog.\n",
			this->wdog_timeout);
	} else
		tinyrl__set_timeout(this->tinyrl, this->idle_timeout);

error:
	return result;
}

/*----------------------------------------------------------- */
int clish_shell_exec_action(clish_action_t *action,
	clish_context_t *context, char **out)
{
	clish_shell_t *this = context->shell;
	int result = 0;
	const char *builtin;
	char *script;

	builtin = clish_action__get_builtin(action);
	script = clish_shell_expand(clish_action__get_script(action), SHELL_VAR_ACTION, context);
	if (builtin) {
		clish_shell_builtin_fn_t *callback;
		lub_argv_t *argv = script ? lub_argv_new(script, 0) : NULL;
		result = -1;
		/* search for an internal command */
		callback = find_builtin_callback(clish_cmd_list, builtin);
		if (!callback) {
			/* search for a client command */
			callback = find_builtin_callback(
				this->client_hooks->cmd_list, builtin);
		}
		/* invoke the builtin callback */
		if (callback)
			result = callback(context, argv, script, out);
		if (argv)
			lub_argv_delete(argv);
	} else if (script) {
		/* now get the client to interpret the resulting script */
		result = this->client_hooks->script_fn(context, action, script, out);
	}
	lub_string_free(script);

	return result;
}

/*----------------------------------------------------------- */
/*
 * Find out the previous view in the stack and go to it
 */
static int clish_nested_up(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	clish_shell_t *this = context->shell;

	if (!this)
		return -1;

	argv = argv; /* not used */

	/* If depth=0 than exit */
	if (0 == this->depth) {
		this->state = SHELL_STATE_CLOSING;
		return 0;
	}
	this->depth--;

	return 0;
}

/*----------------------------------------------------------- */
/*
 * Builtin: NOP function
 */
static int clish_nop(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	return 0;
}

/*----------------------------------------------------------- */
/*
 * Builtin: Set watchdog timeout. The "0" to turn watchdog off.
 */
static int clish_wdog(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	const char *arg = lub_argv__get_arg(argv, 0);
	clish_shell_t *this = context->shell;

	/* Turn off watchdog if no args */
	if (!arg || ('\0' == *arg)) {
		this->wdog_timeout = 0;
		return 0;
	}

	this->wdog_timeout = (unsigned int)atoi(arg);

	return 0;
}

/*--------------------------------------------------------- */
/*
 * Get the ACTION context as a macros
 */
static int clish_macros(clish_context_t *context, const lub_argv_t *argv,
	const char *script, char **out)
{
	if (!script) /* Nothing to do */
		return 0;
	*out = lub_string_dup(script);
	return 0;
}

/*----------------------------------------------------------- */
const char *clish_shell__get_fifo(clish_shell_t * this)
{
	char *name;
	int res;

	if (this->fifo_name) {
		if (0 == access(this->fifo_name, R_OK | W_OK))
			return this->fifo_name;
		unlink(this->fifo_name);
		lub_string_free(this->fifo_name);
		this->fifo_name = NULL;
	}

	do {
		char template[] = "/tmp/klish.fifo.XXXXXX";
		name = mktemp(template);
		if (name[0] == '\0')
			return NULL;
		res = mkfifo(name, 0600);
		if (res == 0)
			this->fifo_name = lub_string_dup(name);
	} while ((res < 0) && (EEXIST == errno));

	return this->fifo_name;
}

/*--------------------------------------------------------- */
void *clish_shell__get_client_cookie(const clish_shell_t * this)
{
	return this->client_cookie;
}

/*-------------------------------------------------------- */
void clish_shell__set_log(clish_shell_t *this, bool_t log)
{
	assert(this);
	this->log = log;
}

/*-------------------------------------------------------- */
bool_t clish_shell__get_log(const clish_shell_t *this)
{
	assert(this);
	return this->log;
}

/*----------------------------------------------------------- */
