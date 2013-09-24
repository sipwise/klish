/*
 * clish_script_callback.c
 *
 * Callback hook to action a shell script.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lub/string.h"
#include "konf/buf.h"
#include "internal.h"

/*--------------------------------------------------------- */
int clish_script_callback(clish_context_t *context,
	clish_action_t *action, const char *script, char **out)
{
	clish_shell_t *this = context->shell;
	const char * shebang = NULL;
	pid_t cpid = -1;
	int res;
	const char *fifo_name;
	FILE *rpipe, *wpipe;
	char *command = NULL;
	bool_t is_sh = BOOL_FALSE;

	/* Signal vars */
	struct sigaction sig_old_int;
	struct sigaction sig_old_quit;
	struct sigaction sig_new;
	sigset_t sig_set;

	assert(this);
	if (!script) /* Nothing to do */
		return BOOL_TRUE;

	/* Find out shebang */
	if (action)
		shebang = clish_action__get_shebang(action);
	if (!shebang)
		shebang = clish_shell__get_default_shebang(this);
	assert(shebang);
	if (0 == lub_string_nocasecmp(shebang, "/bin/sh"))
		is_sh = BOOL_TRUE;

#ifdef DEBUG
	fprintf(stderr, "SHEBANG: #!%s\n", shebang);
	fprintf(stderr, "SCRIPT: %s\n", script);
#endif /* DEBUG */

	/* If /bin/sh we don't need FIFO */
	if (!is_sh) {
		/* Get FIFO */
		fifo_name = clish_shell__get_fifo(this);
		if (!fifo_name) {
			fprintf(stderr, "System error. Can't create temporary FIFO.\n"
				"The ACTION will be not executed.\n");
			return BOOL_FALSE;
		}

		/* Create process to write to FIFO */
		cpid = fork();
		if (cpid == -1) {
			fprintf(stderr, "System error. Can't fork the write process.\n"
				"The ACTION will be not executed.\n");
			return BOOL_FALSE;
		}

		/* Child: write to FIFO */
		if (cpid == 0) {
			wpipe = fopen(fifo_name, "w");
			if (!wpipe)
				_exit(-1);
			fwrite(script, strlen(script) + 1, 1, wpipe);
			fclose(wpipe);
			_exit(0);
		}
	}

	/* Parent */
	/* Prepare command */
	if (!is_sh) {
		lub_string_cat(&command, shebang);
		lub_string_cat(&command, " ");
		lub_string_cat(&command, fifo_name);
	} else {
		lub_string_cat(&command, script);
	}

	/* If the stdout of script is needed */
	if (out) {
		konf_buf_t *buf;

		/* Ignore SIGINT and SIGQUIT */
		sigemptyset(&sig_set);
		sig_new.sa_flags = 0;
		sig_new.sa_mask = sig_set;
		sig_new.sa_handler = SIG_IGN;
		sigaction(SIGINT, &sig_new, &sig_old_int);
		sigaction(SIGQUIT, &sig_new, &sig_old_quit);

		/* Execute shebang with FIFO as argument */
		rpipe = popen(command, "r");
		if (!rpipe) {
			fprintf(stderr, "System error. Can't fork the script.\n"
				"The ACTION will be not executed.\n");
			lub_string_free(command);
			if (!is_sh) {
				kill(cpid, SIGTERM);
				waitpid(cpid, NULL, 0);
			}

			/* Restore SIGINT and SIGQUIT */
			sigaction(SIGINT, &sig_old_int, NULL);
			sigaction(SIGQUIT, &sig_old_quit, NULL);

			return BOOL_FALSE;
		}
		/* Read the result of script execution */
		buf = konf_buf_new(fileno(rpipe));
		while (konf_buf_read(buf) > 0);
		*out = konf_buf__dup_line(buf);
		konf_buf_delete(buf);
		/* Wait for the writing process */
		if (!is_sh) {
			kill(cpid, SIGTERM);
			waitpid(cpid, NULL, 0);
		}
		/* Wait for script */
		res = pclose(rpipe);

		/* Restore SIGINT and SIGQUIT */
		sigaction(SIGINT, &sig_old_int, NULL);
		sigaction(SIGQUIT, &sig_old_quit, NULL);
	} else {
		res = system(command);
		/* Wait for the writing process */
		if (!is_sh) {
			kill(cpid, SIGTERM);
			waitpid(cpid, NULL, 0);
		}
	}
	lub_string_free(command);

#ifdef DEBUG
	fprintf(stderr, "RETCODE: %d\n", WEXITSTATUS(res));
#endif /* DEBUG */
	return WEXITSTATUS(res);
}

/*--------------------------------------------------------- */
int clish_dryrun_callback(clish_context_t *context,
	clish_action_t *action, const char *script, char ** out)
{
#ifdef DEBUG
	fprintf(stderr, "DRY-RUN: %s\n", script);
#endif /* DEBUG */
	if (out)
		*out = NULL;

	return 0;
}

/*--------------------------------------------------------- */
