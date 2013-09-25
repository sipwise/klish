/*
 * context.c
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
int clish_context_init(clish_context_t *this, clish_shell_t *shell)
{
	if (!this)
		return -1;
	memset(this, 0, sizeof(*this));
	this->shell = shell;

	return 0;
}

/*--------------------------------------------------------- */
clish_context_t *clish_context_new(clish_shell_t *shell)
{
	clish_context_t *this;

	if (!shell)
		return NULL;
	if (!(this = malloc(sizeof(*this))))
		return NULL;
	clish_context_init(this, shell);

	return this;
}

/*--------------------------------------------------------- */
/* Note it will not free all content because it's a
 * container only.
 */
void clish_context_free(clish_context_t *this)
{
	if (!this)
		return;
	free(this);
}

/*--------------------------------------------------------- */
clish_shell_t *clish_context__get_shell(const void *this)
{
	const clish_context_t *context = (const clish_context_t *)this;
	return context->shell;
}

/*--------------------------------------------------------- */
void clish_context__set_cmd(void *this, const clish_command_t *cmd)
{
	clish_context_t *context = (clish_context_t *)this;
	assert(context);
	context->cmd = cmd;
}

/*--------------------------------------------------------- */
const clish_command_t *clish_context__get_cmd(const void *this)
{
	const clish_context_t *context = (const clish_context_t *)this;
	return context->cmd;
}

/*--------------------------------------------------------- */
void clish_context__set_pargv(void *this, clish_pargv_t *pargv)
{
	clish_context_t *context = (clish_context_t *)this;
	assert(context);
	context->pargv = pargv;
}

/*--------------------------------------------------------- */
clish_pargv_t *clish_context__get_pargv(const void *this)
{
	const clish_context_t *context = (const clish_context_t *)this;
	return context->pargv;
}

/*--------------------------------------------------------- */
void clish_context__set_action(void *this, const clish_action_t *action)
{
	clish_context_t *context = (clish_context_t *)this;
	assert(context);
	context->action = action;
}

/*--------------------------------------------------------- */
const clish_action_t *clish_context__get_action(const void *this)
{
	const clish_context_t *context = (const clish_context_t *)this;
	return context->action;
}

/*--------------------------------------------------------- */
