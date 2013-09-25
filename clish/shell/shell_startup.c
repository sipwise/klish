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

	if (!this->startup) {
		fprintf(stderr, "Error: Can't get valid STARTUP tag.\n");
		return -1;
	}

	/* Prepare context */
	clish_context_init(&context, this);
	clish_context__set_cmd(&context, this->startup);
	clish_context__set_action(&context,
		clish_command__get_action(this->startup));

	banner = clish_command__get_detail(this->startup);
	if (banner)
		tinyrl_printf(this->tinyrl, "%s\n", banner);

	/* Call log initialize */
	if (clish_shell__get_log(this))
		clish_shell_exec_log(&context, NULL, 0);
	/* Call startup script */
	return clish_shell_execute(&context, NULL);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_view(clish_shell_t *this, const char *viewname)
{
	assert(this);
	assert(this->startup);
	clish_command__force_viewname(this->startup, viewname);
}

/*----------------------------------------------------------- */
void clish_shell__set_startup_viewid(clish_shell_t *this, const char *viewid)
{
	assert(this);
	assert(this->startup);
	clish_command__force_viewid(this->startup, viewid);
}

/*----------------------------------------------------------- */
void clish_shell__set_default_shebang(clish_shell_t *this, const char *shebang)
{
	assert(this);
	lub_string_free(this->default_shebang);
	this->default_shebang = lub_string_dup(shebang);
}

/*----------------------------------------------------------- */
const char * clish_shell__get_default_shebang(const clish_shell_t *this)
{
	assert(this);
	return this->default_shebang;
}

/*----------------------------------------------------------- */
