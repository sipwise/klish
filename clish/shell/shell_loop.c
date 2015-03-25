/*
 * shell_loop.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/*-------------------------------------------------------- */
/* The main CLI loop while interactive execution.
 */
int clish_shell_loop(clish_shell_t *this)
{
	int running = 0;
	int retval = SHELL_STATE_OK;

	assert(this);
	if (!tinyrl__get_istream(this->tinyrl))
		return SHELL_STATE_IO_ERROR;
	/* Check the shell isn't closing down */
	if (this && (SHELL_STATE_CLOSING == this->state))
		return retval;

	/* Loop reading and executing lines until the user quits */
	while (!running) {
		retval = SHELL_STATE_OK;
		/* Get input from the stream */
		running = clish_shell_readline(this, NULL);
		if (running) {
			switch (this->state) {
			case SHELL_STATE_SCRIPT_ERROR:
			case SHELL_STATE_SYNTAX_ERROR:
				/* Interactive session doesn't exit on error */
				if (tinyrl__get_isatty(this->tinyrl) ||
					(this->current_file &&
					!this->current_file->stop_on_error))
					running = 0;
				retval = this->state;
			default:
				break;
			}
		}
		if (SHELL_STATE_CLOSING == this->state)
			running = -1;
		if (running)
			running = clish_shell_pop_file(this);
	}

	return retval;
}

/*-------------------------------------------------------- */
