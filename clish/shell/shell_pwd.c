/*
 * shell_pwd.c
 */
#include <stdlib.h>
#include <assert.h>

#include "lub/string.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_shell__init_pwd(clish_shell_pwd_t *pwd)
{
	pwd->line = NULL;
	pwd->view = NULL;
	/* initialise the tree of vars */
	lub_bintree_init(&pwd->viewid,
		clish_var_bt_offset(),
		clish_var_bt_compare, clish_var_bt_getkey);
}

/*--------------------------------------------------------- */
void clish_shell__fini_pwd(clish_shell_pwd_t *pwd)
{
	clish_var_t *var;

	lub_string_free(pwd->line);
	pwd->view = NULL;
	/* delete each VAR held  */
	while ((var = lub_bintree_findfirst(&pwd->viewid))) {
		lub_bintree_remove(&pwd->viewid, var);
		clish_var_delete(var);
	}
}

/*--------------------------------------------------------- */
void clish_shell__set_pwd(clish_shell_t *this,
	const char *line, clish_view_t *view, char *viewid, clish_context_t *context)
{
	clish_shell_pwd_t **tmp;
	size_t new_size = 0;
	unsigned int i;
	unsigned int index = clish_view__get_depth(view);
	clish_shell_pwd_t *newpwd;

	/* Create new element */
	newpwd = malloc(sizeof(*newpwd));
	assert(newpwd);
	clish_shell__init_pwd(newpwd);

	/* Resize the pwd vector */
	if (index >= this->pwdc) {
		new_size = (index + 1) * sizeof(clish_shell_pwd_t *);
		tmp = realloc(this->pwdv, new_size);
		assert(tmp);
		this->pwdv = tmp;
		/* Initialize new elements */
		for (i = this->pwdc; i <= index; i++) {
			clish_shell_pwd_t *pwd = malloc(sizeof(*pwd));
			assert(pwd);
			clish_shell__init_pwd(pwd);
			this->pwdv[i] = pwd;
		}
		this->pwdc = index + 1;
	}

	/* Fill the new pwd entry */
	newpwd->line = line ? lub_string_dup(line) : NULL;
	newpwd->view = view;
	clish_shell__expand_viewid(viewid, &newpwd->viewid, context);
	clish_shell__fini_pwd(this->pwdv[index]);
	free(this->pwdv[index]);
	this->pwdv[index] = newpwd;
	this->depth = index;
}

/*--------------------------------------------------------- */
char *clish_shell__get_pwd_line(const clish_shell_t *this, unsigned int index)
{
	if (index >= this->pwdc)
		return NULL;

	return this->pwdv[index]->line;
}

/*--------------------------------------------------------- */
char *clish_shell__get_pwd_full(const clish_shell_t * this, unsigned depth)
{
	char *pwd = NULL;
	unsigned i;

	for (i = 1; i <= depth; i++) {
		const char *str =
			clish_shell__get_pwd_line(this, i);
		/* Cannot get full path */
		if (!str) {
			lub_string_free(pwd);
			return NULL;
		}
		if (pwd)
			lub_string_cat(&pwd, " ");
		lub_string_cat(&pwd, "\"");
		lub_string_cat(&pwd, str);
		lub_string_cat(&pwd, "\"");
	}

	return pwd;
}

/*--------------------------------------------------------- */
clish_view_t *clish_shell__get_pwd_view(const clish_shell_t * this, unsigned int index)
{
	if (index >= this->pwdc)
		return NULL;

	return this->pwdv[index]->view;
}

/*--------------------------------------------------------- */
konf_client_t *clish_shell__get_client(const clish_shell_t * this)
{
	return this->client;
}

/*--------------------------------------------------------- */
void clish_shell__set_lockfile(clish_shell_t * this, const char * path)
{
	if (!this)
		return;

	lub_string_free(this->lockfile);
	this->lockfile = NULL;
	if (path)
		this->lockfile = lub_string_dup(path);
}

/*--------------------------------------------------------- */
char * clish_shell__get_lockfile(clish_shell_t * this)
{
	if (!this)
		return NULL;

	return this->lockfile;
}

/*--------------------------------------------------------- */
int clish_shell__set_socket(clish_shell_t * this, const char * path)
{
	if (!this || !path)
		return -1;

	konf_client_free(this->client);
	this->client = konf_client_new(path);

	return 0;
}
