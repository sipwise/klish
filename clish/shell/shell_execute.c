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
#include <sys/wait.h>
#include <sys/uio.h>
#include <signal.h>
#include <fcntl.h>

/* Empty signal handler to ignore signal but don't use SIG_IGN. */
static void sigignore(int signo)
{
	signo = signo; /* Happy compiler */
	return;
}

/*-------------------------------------------------------- */
static int clish_shell_lock(const char *lock_path)
{
	int i;
	int res = -1;
	int lock_fd = -1;
	struct flock lock;

	if (!lock_path)
		return -1;
	lock_fd = open(lock_path, O_WRONLY | O_CREAT, 00644);
	if (-1 == lock_fd) {
		fprintf(stderr, "Warning: Can't open lockfile %s.\n", lock_path);
		return -1;
	}
#ifdef FD_CLOEXEC
	fcntl(lock_fd, F_SETFD, fcntl(lock_fd, F_GETFD) | FD_CLOEXEC);
#endif
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	for (i = 0; i < CLISH_LOCK_WAIT; i++) {
		res = fcntl(lock_fd, F_SETLK, &lock);
		if (res != -1)
			break;
		if (EINTR == errno)
			continue;
		if ((EAGAIN == errno) || (EACCES == errno)) {
			if (0 == i)
				fprintf(stderr,
					"Warning: Try to get lock. Please wait...\n");
			sleep(1);
			continue;
		}
		if (EINVAL == errno)
			fprintf(stderr, "Error: Locking isn't supported by OS, consider \"--lockless\".\n");
		break;
	}
	if (res == -1) {
		fprintf(stderr, "Error: Can't get lock.\n");
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
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	fcntl(lock_fd, F_SETLK, &lock);
	close(lock_fd);
}

/*----------------------------------------------------------- */
int clish_shell_execute(clish_context_t *context, char **out)
{
	clish_shell_t *this = clish_context__get_shell(context);
	const clish_command_t *cmd = clish_context__get_cmd(context);
	int result = 0;
	char *lock_path = clish_shell__get_lockfile(this);
	int lock_fd = -1;
	clish_view_t *cur_view = clish_shell__get_view(this);
	unsigned int saved_wdog_timeout = this->wdog_timeout;

	assert(cmd);

	/* Pre-change view if the command is from another depth/view */
	{
		clish_view_restore_e restore = clish_command__get_restore(cmd);
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

	/* Execute ACTION */
	clish_context__set_action(context, clish_command__get_action(cmd));
	result = clish_shell_exec_action(context, out,
		clish_command__get_interrupt(cmd));

	/* Call config callback */
	if (!result)
		clish_shell_exec_config(context);

	/* Call logging callback */
	if (clish_shell__get_log(this) &&
		clish_shell_check_hook(context, CLISH_SYM_TYPE_LOG)) {
		char *full_line = clish_shell__get_full_line(context);
		clish_shell_exec_log(context, full_line, result);
		lub_string_free(full_line);
	}

	/* Unlock the lockfile */
	if (lock_fd != -1)
		clish_shell_unlock(lock_fd);

	/* Move into the new view */
	if (!result) {
		char *viewname = clish_shell_expand(clish_command__get_viewname(cmd), SHELL_VAR_NONE, context);
		if (viewname) {
			/* Search for the view */
			clish_view_t *view = clish_shell_find_view(this, viewname);
			if (!view)
				fprintf(stderr, "System error: Can't "
					"change view to %s\n", viewname);
			lub_string_free(viewname);
			/* Save the PWD */
			if (view) {
				char *line = clish_shell__get_line(context);
				clish_shell__set_pwd(this, line, view,
					clish_command__get_viewid(cmd), context);
				lub_string_free(line);
			}
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
/* Execute oaction. It suppose the forked process to get
 * script's stdout. Then forked process write the output back
 * to klish.
 */
static int clish_shell_exec_oaction(clish_hook_oaction_fn_t func,
	void *context, const char *script, char **out)
{
	int result = -1;
	int real_stdout; /* Saved stdout handler */
	int pipe1[2], pipe2[2];
	pid_t cpid = -1;
	konf_buf_t *buf;

	if (pipe(pipe1))
		return -1;
	if (pipe(pipe2))
		goto stdout_error;

	/* Create process to read script's stdout */
	cpid = fork();
	if (cpid == -1) {
		fprintf(stderr, "Error: Can't fork the stdout-grabber process.\n"
			"Error: The ACTION will be not executed.\n");
		goto stdout_error;
	}

	/* Child: read action's stdout */
	if (cpid == 0) {
		lub_list_t *l;
		lub_list_node_t *node;
		struct iovec *iov;
		const int rsize = CLISH_STDOUT_CHUNK; /* Read chunk size */
		size_t cur_size = 0;

		close(pipe1[1]);
		close(pipe2[0]);
		l = lub_list_new(NULL);

		/* Read the result of script execution */
		while (1) {
			ssize_t ret;
			iov = malloc(sizeof(*iov));
			iov->iov_len = rsize;
			iov->iov_base = malloc(iov->iov_len);
			do {
				ret = readv(pipe1[0], iov, 1);
			} while ((ret < 0) && (errno == EINTR));
			if (ret <= 0) { /* Error or EOF */
				free(iov->iov_base);
				free(iov);
				break;
			}
			iov->iov_len = ret;
			lub_list_add(l, iov);
			/* Check the max size of buffer */
			cur_size += ret;
			if (cur_size >= CLISH_STDOUT_MAXBUF)
				break;
		}
		close(pipe1[0]);

		/* Write the result of script back to klish */
		while ((node = lub_list__get_head(l))) {
			iov = lub_list_node__get_data(node);
			lub_list_del(l, node);
			lub_list_node_free(node);
			write(pipe2[1], iov->iov_base, iov->iov_len);
			free(iov->iov_base);
			free(iov);
		}
		close(pipe2[1]);

		lub_list_free(l);
		_exit(0);
	}

	real_stdout = dup(STDOUT_FILENO);
	dup2(pipe1[1], STDOUT_FILENO);
	close(pipe1[0]);
	close(pipe1[1]);
	close(pipe2[1]);

	result = func(context, script);

	/* Restore real stdout */
	dup2(real_stdout, STDOUT_FILENO);
	close(real_stdout);
	/* Read the result of script execution */
	buf = konf_buf_new(pipe2[0]);
	while (konf_buf_read(buf) > 0);
	*out = konf_buf__dup_line(buf);
	konf_buf_delete(buf);
	close(pipe2[0]);
	/* Wait for the stdout-grabber process */
	waitpid(cpid, NULL, 0);

	return result;

stdout_error:
	close(pipe1[0]);
	close(pipe1[1]);
	return -1;
}

/*----------------------------------------------------------- */
int clish_shell_exec_action(clish_context_t *context, char **out, bool_t intr)
{
	int result = -1;
	clish_sym_t *sym;
	char *script;
	void *func = NULL; /* We don't know the func API at this time */
	const clish_action_t *action = clish_context__get_action(context);
	clish_shell_t *shell = clish_context__get_shell(context);
	/* Signal vars */
	struct sigaction old_sigint, old_sigquit, old_sighup;
	struct sigaction sa;
	sigset_t old_sigs;

	if (!(sym = clish_action__get_builtin(action)))
		return 0;
	if (shell->dryrun && !clish_sym__get_permanent(sym))
		return 0;
	if (!(func = clish_sym__get_func(sym))) {
		fprintf(stderr, "Error: Default ACTION symbol is not specified.\n");
		return -1;
	}
	script = clish_shell_expand(clish_action__get_script(action), SHELL_VAR_ACTION, context);

	/* Ignore and block SIGINT, SIGQUIT, SIGHUP.
	 * The SIG_IGN is not a case because it will be inherited
	 * while a fork(). It's necessary to ignore signals because
	 * the klish itself and ACTION script share the same terminal.
	 */
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigignore; /* Empty signal handler */
	sigaction(SIGINT, &sa, &old_sigint);
	sigaction(SIGQUIT, &sa, &old_sigquit);
	sigaction(SIGHUP, &sa, &old_sighup);
	/* Block signals for children processes. The block state is inherited. */
	if (!intr) {
		sigset_t sigs;
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGINT);
		sigaddset(&sigs, SIGQUIT);
		sigaddset(&sigs, SIGHUP);
		sigprocmask(SIG_BLOCK, &sigs, &old_sigs);
	}

	/* Find out the function API */
	/* CLISH_SYM_API_SIMPLE */
	if (clish_sym__get_api(sym) == CLISH_SYM_API_SIMPLE) {
		result = ((clish_hook_action_fn_t *)func)(context, script, out);

	/* CLISH_SYM_API_STDOUT and output is not needed */
	} else if ((clish_sym__get_api(sym) == CLISH_SYM_API_STDOUT) && (!out)) {
		result = ((clish_hook_oaction_fn_t *)func)(context, script);

	/* CLISH_SYM_API_STDOUT and outpus is needed */
	} else if (clish_sym__get_api(sym) == CLISH_SYM_API_STDOUT) {
		result = clish_shell_exec_oaction((clish_hook_oaction_fn_t *)func,
			context, script, out);
	}

	/* Restore SIGINT, SIGQUIT, SIGHUP */
	if (!intr) {
		sigprocmask(SIG_SETMASK, &old_sigs, NULL);
		/* Is the signals delivery guaranteed here (before
		   sigaction restore) for previously blocked and
		   pending signals? The simple test is working well.
		   I don't want to use sigtimedwait() function because
		   it needs a realtime extensions. The sigpending() with
		   the sleep() is not nice too. Report bug if clish will
		   get the SIGINT after non-interruptable action.
		*/
	}
	sigaction(SIGINT, &old_sigint, NULL);
	sigaction(SIGQUIT, &old_sigquit, NULL);
	sigaction(SIGHUP, &old_sighup, NULL);

	lub_string_free(script);

	return result;
}

/*----------------------------------------------------------- */
void *clish_shell_check_hook(const clish_context_t *clish_context, int type)
{
	clish_sym_t *sym;
	clish_shell_t *shell = clish_context__get_shell(clish_context);
	void *func;

	if (!(sym = shell->hooks[type]))
		return NULL;
	if (shell->dryrun && !clish_sym__get_permanent(sym))
		return NULL;
	if (!(func = clish_sym__get_func(sym)))
		return NULL;

	return func;
}

/*----------------------------------------------------------- */
CLISH_HOOK_CONFIG(clish_shell_exec_config)
{
	clish_hook_config_fn_t *func = NULL;
	func = clish_shell_check_hook(clish_context, CLISH_SYM_TYPE_CONFIG);
	return func ? func(clish_context) : 0;
}

/*----------------------------------------------------------- */
CLISH_HOOK_LOG(clish_shell_exec_log)
{
	clish_hook_log_fn_t *func = NULL;
	func = clish_shell_check_hook(clish_context, CLISH_SYM_TYPE_LOG);
	return func ? func(clish_context, line, retcode) : 0;
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

/*-------------------------------------------------------- */
void clish_shell__set_dryrun(clish_shell_t *this, bool_t dryrun)
{
	this->dryrun = dryrun;
}

/*-------------------------------------------------------- */
bool_t clish_shell__get_dryrun(const clish_shell_t *this)
{
	return this->dryrun;
}

/*----------------------------------------------------------- */
