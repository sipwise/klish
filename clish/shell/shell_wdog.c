/*
 * shell_wdog.c
 */
#include "private.h"
#include <assert.h>

#include "tinyrl/tinyrl.h"
#include "lub/string.h"

/*----------------------------------------------------------------------- */
int clish_shell_timeout_fn(tinyrl_t *tinyrl)
{
	clish_context_t *context = tinyrl__get_context(tinyrl);
	clish_shell_t *this = clish_context__get_shell(context);

	/* Idle timeout */
	if (!this->wdog_active) {
		tinyrl_crlf(tinyrl);
		fprintf(stderr, "Warning: Idle timeout. The session will be closed.\n");
		/* Return -1 to close session on timeout */
		return -1;
	}

	/* Watchdog timeout */
	clish_shell_wdog(this);
	this->wdog_active = BOOL_FALSE;
	tinyrl__set_timeout(tinyrl, this->idle_timeout);

	return 0;
}

/*----------------------------------------------------------------------- */
int clish_shell_keypress_fn(tinyrl_t *tinyrl, int key)
{
	clish_context_t *context = tinyrl__get_context(tinyrl);
	clish_shell_t *this = clish_context__get_shell(context);

	if (this->wdog_active) {
		this->wdog_active = BOOL_FALSE;
		tinyrl__set_timeout(tinyrl, this->idle_timeout);
	}

	key = key; /* Happy compiler */

	return 0;
}

/*----------------------------------------------------------- */
int clish_shell_wdog(clish_shell_t *this)
{
	clish_context_t context;

	assert(this->wdog);

	/* Prepare context */
	clish_context_init(&context, this);
	clish_context__set_cmd(&context, this->wdog);
	clish_context__set_action(&context,
		clish_command__get_action(this->wdog));

	/* Call watchdog script */
	return clish_shell_execute(&context, NULL);
}

/*----------------------------------------------------------- */
void clish_shell__set_wdog_timeout(clish_shell_t *this, unsigned int timeout)
{
	assert(this);
	this->wdog_timeout = timeout;
}

/*----------------------------------------------------------- */
unsigned int clish_shell__get_wdog_timeout(const clish_shell_t *this)
{
	assert(this);
	return this->wdog_timeout;
}
