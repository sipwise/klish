/*
 * shell_new.c
 */
#include "private.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "lub/string.h"
#include "lub/db.h"

/*-------------------------------------------------------- */
static void clish_shell_init(clish_shell_t * this,
	const clish_shell_hooks_t * hooks,
	void *cookie, FILE * istream,
	FILE * ostream,
	bool_t stop_on_error)
{
	clish_ptype_t *tmp_ptype = NULL;

	/* initialise the tree of views */
	lub_bintree_init(&this->view_tree,
		clish_view_bt_offset(),
		clish_view_bt_compare, clish_view_bt_getkey);

	/* initialise the tree of ptypes */
	lub_bintree_init(&this->ptype_tree,
		clish_ptype_bt_offset(),
		clish_ptype_bt_compare, clish_ptype_bt_getkey);

	/* initialise the tree of vars */
	lub_bintree_init(&this->var_tree,
		clish_var_bt_offset(),
		clish_var_bt_compare, clish_var_bt_getkey);

	assert((NULL != hooks) && (NULL != hooks->script_fn));

	/* set up defaults */
	this->client_hooks = hooks;
	this->client_cookie = cookie;
	this->global = NULL;
	this->startup = NULL;
	this->idle_timeout = 0; /* No idle timeout by default */
	this->wdog = NULL;
	this->wdog_timeout = 0; /* No watchdog timeout by default */
	this->wdog_active = BOOL_FALSE;
	this->state = SHELL_STATE_INITIALISING;
	this->overview = NULL;
	this->tinyrl = clish_shell_tinyrl_new(istream, ostream, 0);
	this->current_file = NULL;
	this->pwdv = NULL;
	this->pwdc = 0;
	this->depth = -1; /* Current depth is undefined */
	this->client = NULL;
	this->lockfile = lub_string_dup(CLISH_LOCK_PATH);
	this->default_shebang = lub_string_dup("/bin/sh");
	this->fifo_name = NULL;
	this->interactive = BOOL_TRUE; /* The interactive shell by default. */
	this->log = BOOL_FALSE; /* Disable logging by default */
	this->user = lub_db_getpwuid(getuid()); /* Get user information */

	/* Create internal ptypes and params */
	/* Current depth */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__DEPTH", "Depth", "[0-9]+",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
	assert(tmp_ptype);
	this->param_depth = clish_param_new("_cur_depth",
		"Current depth", tmp_ptype);
	clish_param__set_hidden(this->param_depth, BOOL_TRUE);
	/* Current pwd */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"__PWD", "Path", ".+",
		CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
	assert(tmp_ptype);
	this->param_pwd = clish_param_new("_cur_pwd",
		"Current path", tmp_ptype);
	clish_param__set_hidden(this->param_pwd, BOOL_TRUE);
	/* Args */
	tmp_ptype = clish_shell_find_create_ptype(this,
		"internal_ARGS",
		"Arguments", "[^\\\\]+",
		CLISH_PTYPE_REGEXP,
		CLISH_PTYPE_NONE);
	assert(tmp_ptype);

	/* Push non-NULL istream */
	if (istream)
		clish_shell_push_fd(this, istream, stop_on_error);
}

/*--------------------------------------------------------- */
static void clish_shell_fini(clish_shell_t * this)
{
	clish_view_t *view;
	clish_ptype_t *ptype;
	clish_var_t *var;
	unsigned i;

	/* delete each VIEW held  */
	while ((view = lub_bintree_findfirst(&this->view_tree))) {
		lub_bintree_remove(&this->view_tree, view);
		clish_view_delete(view);
	}

	/* delete each PTYPE held  */
	while ((ptype = lub_bintree_findfirst(&this->ptype_tree))) {
		lub_bintree_remove(&this->ptype_tree, ptype);
		clish_ptype_delete(ptype);
	}

	/* delete each VAR held  */
	while ((var = lub_bintree_findfirst(&this->var_tree))) {
		lub_bintree_remove(&this->var_tree, var);
		clish_var_delete(var);
	}

	/* free the textual details */
	lub_string_free(this->overview);

	/* Remove the startup command */
	if (this->startup)
		clish_command_delete(this->startup);
	/* Remove the watchdog command */
	if (this->wdog)
		clish_command_delete(this->wdog);
	/* clean up the file stack */
	while (!clish_shell_pop_file(this));
	/* delete the tinyrl object */
	clish_shell_tinyrl_delete(this->tinyrl);

	/* finalize each of the pwd strings */
	for (i = 0; i < this->pwdc; i++) {
		clish_shell__fini_pwd(this->pwdv[i]);
		free(this->pwdv[i]);
	}
	/* free the pwd vector */
	free(this->pwdv);
	konf_client_free(this->client);

	/* Free internal params */
	clish_param_delete(this->param_depth);
	clish_param_delete(this->param_pwd);

	lub_string_free(this->lockfile);
	lub_string_free(this->default_shebang);
	free(this->user);
	if (this->fifo_name) {
		unlink(this->fifo_name);
		lub_string_free(this->fifo_name);
	}
}

/*-------------------------------------------------------- */
clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
	void *cookie,
	FILE * istream,
	FILE * ostream,
	bool_t stop_on_error)
{
	clish_shell_t *this = malloc(sizeof(clish_shell_t));

	if (this) {
		clish_shell_init(this, hooks, cookie,
			istream, ostream, stop_on_error);
		if (hooks->init_fn) {
			/* now call the client initialisation */
			if (BOOL_TRUE != hooks->init_fn(this))
				this->state = SHELL_STATE_CLOSING;
		}
	}

	return this;
}

/*--------------------------------------------------------- */
void clish_shell_delete(clish_shell_t * this)
{
	/* now call the client finalisation */
	if (this->client_hooks->fini_fn)
		this->client_hooks->fini_fn(this);
	clish_shell_fini(this);

	free(this);
}

/*--------------------------------------------------------- */
struct passwd *clish_shell__get_user(clish_shell_t * this)
{
	return this->user;
}

/*-------------------------------------------------------- */
