/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

#include "lub/string.h"

/*----------------------------------------------------------- */
int clish_shell_startup(clish_shell_t *this)
{
	const char *banner;
	clish_context_t context;
	int res = 0;

	assert(this->startup);
	banner = clish_command__get_detail(this->startup);
	if (banner)
		tinyrl_printf(this->tinyrl, "%s\n", banner);

	context.shell = this;
	context.cmd = this->startup;
	context.pargv = NULL;
	/* Call log initialize */
	if (clish_shell__get_log(this) && this->client_hooks->log_fn)
		this->client_hooks->log_fn(&context, NULL, 0);
	/* Call startup script */
	res = clish_shell_execute(&context, NULL);

	return res;
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_view(clish_shell_t * this, const char * viewname)
{
	clish_view_t *view;

	assert(this);
	assert(this->startup);
	/* Search for the view */
	view = clish_shell_find_view(this, viewname);
	assert(view);
	clish_command__force_view(this->startup, viewname);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_viewid(clish_shell_t * this, const char * viewid)
{
	assert(this);
	assert(this->startup);
	clish_command__force_viewid(this->startup, viewid);
}

/*----------------------------------------------------------- */
void clish_shell__set_default_shebang(clish_shell_t * this, const char * shebang)
{
	assert(this);
	lub_string_free(this->default_shebang);
	this->default_shebang = lub_string_dup(shebang);
}

/*----------------------------------------------------------- */
const char * clish_shell__get_default_shebang(const clish_shell_t * this)
{
	assert(this);
	return this->default_shebang;
}

/*----------------------------------------------------------- */
