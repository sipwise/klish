/*
 * plugin.c
 */
#include "private.h"
#include "lub/string.h"
#include "lub/list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>

/**********************************************************
 * SYM functions                                          *
 **********************************************************/

/*--------------------------------------------------------- */
int clish_sym_compare(const void *first, const void *second)
{
	const clish_sym_t *f = (const clish_sym_t *)first;
	const clish_sym_t *s = (const clish_sym_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_sym_new(const char *name, void *func, int type)
{
	clish_sym_t *this;

	this = malloc(sizeof(*this));
	this->name = lub_string_dup(name);
	this->func = func;
	this->type = type;
	this->permanent = BOOL_FALSE;

	return this;
}

/*--------------------------------------------------------- */
void clish_sym_free(clish_sym_t *this)
{
	if (!this)
		return;
	lub_string_free(this->name);
	free(this);
}

/*--------------------------------------------------------- */
void clish_sym__set_func(clish_sym_t *this, void *func)
{
	this->func = func;
}

/*--------------------------------------------------------- */
void *clish_sym__get_func(clish_sym_t *this)
{
	return this->func;
}

/*--------------------------------------------------------- */
void clish_sym__set_permanent(clish_sym_t *this, bool_t permanent)
{
	this->permanent = permanent;
}

/*--------------------------------------------------------- */
bool_t clish_sym__get_permanent(clish_sym_t *this)
{
	return this->permanent;
}

/*--------------------------------------------------------- */
void clish_sym__set_name(clish_sym_t *this, const char *name)
{
	lub_string_free(this->name);
	this->name = lub_string_dup(name);
}

/*--------------------------------------------------------- */
char *clish_sym__get_name(clish_sym_t *this)
{
	return this->name;
}

/*--------------------------------------------------------- */
void clish_sym__set_plugin(clish_sym_t *this, clish_plugin_t *plugin)
{
	this->plugin = plugin;
}

/*--------------------------------------------------------- */
clish_plugin_t *clish_sym__get_plugin(clish_sym_t *this)
{
	return this->plugin;
}

/*--------------------------------------------------------- */
void clish_sym__set_type(clish_sym_t *this, int type)
{
	this->type = type;
}

/*--------------------------------------------------------- */
int clish_sym__get_type(clish_sym_t *this)
{
	return this->type;
}

/*--------------------------------------------------------- */
int clish_sym_clone(clish_sym_t *dst, clish_sym_t *src)
{
	char *name;

	if (!dst || !src)
		return -1;
	name = dst->name;
	*dst = *src;
	dst->name = name;

	return 0;
}

/**********************************************************
 * PLUGIN functions                                       *
 **********************************************************/

/*--------------------------------------------------------- */
clish_plugin_t *clish_plugin_new(const char *file, const char *alias)
{
	clish_plugin_t *this;

	this = malloc(sizeof(*this));

	this->file = lub_string_dup(file);
	this->name = NULL;
	this->conf = NULL;
	this->alias = lub_string_dup(alias);
	this->dlhan = NULL;
	/* Initialise the list of symbols */
	this->syms = lub_list_new(clish_sym_compare);
	/* Constructor and destructor */
	this->init = NULL;
	this->fini = NULL;

	return this;
}

/*--------------------------------------------------------- */
void clish_plugin_free(clish_plugin_t *this, void *userdata)
{
	lub_list_node_t *iter;

	if (!this)
		return;

	/* Execute destructor */
	if (this->fini)
		this->fini(userdata, this);

	lub_string_free(this->file);
	lub_string_free(this->name);
	lub_string_free(this->alias);
	lub_string_free(this->conf);

	/* Free symbol list */
	while ((iter = lub_list__get_head(this->syms))) {
		/* Remove the symbol from the list */
		lub_list_del(this->syms, iter);
		/* Free the instance */
		clish_sym_free((clish_sym_t *)lub_list_node__get_data(iter));
		lub_list_node_free(iter);
	}
	lub_list_free(this->syms);
	if (this->dlhan)
		dlclose(this->dlhan);

	free(this);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_generic(clish_plugin_t *this,
	void *func, const char *name, int type, bool_t permanent)
{
	clish_sym_t *sym;

	if (!name || !func)
		return NULL;

	if (!(sym = clish_sym_new(name, func, type)))
		return NULL;
	clish_sym__set_plugin(sym, this);
	clish_sym__set_permanent(sym, permanent);
	lub_list_add(this->syms, sym);

	return sym;
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_sym(clish_plugin_t *this,
	clish_hook_action_fn_t *func, const char *name)
{
	return clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_FALSE);
}

/*--------------------------------------------------------- */
/* Add permanent symbol (can't be turned off by dry-run) */
clish_sym_t *clish_plugin_add_psym(clish_plugin_t *this,
	clish_hook_action_fn_t *func, const char *name)
{
	return clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_TRUE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_hook(clish_plugin_t *this,
	void *func, const char *name, int type)
{
	return clish_plugin_add_generic(this, func,
		name, type, BOOL_FALSE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_add_phook(clish_plugin_t *this,
	void *func, const char *name, int type)
{
	return clish_plugin_add_generic(this, func,
		name, type, BOOL_TRUE);
}

/*--------------------------------------------------------- */
clish_sym_t *clish_plugin_get_sym(clish_plugin_t *this, const char *name, int type)
{
	lub_list_node_t *iter;
	clish_sym_t *sym;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		int res;
		sym = (clish_sym_t *)lub_list_node__get_data(iter);
		res = strcmp(clish_sym__get_name(sym), name);
		if (!res && ((CLISH_SYM_TYPE_NONE == type) || (clish_sym__get_type(sym) == type)))
			return sym;
		if (res > 0) /* No chances to find name */
			break;
	}

	return NULL;
}

/*--------------------------------------------------------- */
int clish_plugin_load(clish_plugin_t *this, void *userdata)
{
	int res;

	if (!this)
		return -1;

	/* Open dynamic library */
	if (!(this->dlhan = dlopen(this->file, RTLD_NOW | RTLD_LOCAL))) {
		fprintf(stderr, "Error: Can't open plugin %s: %s\n",
			this->file, dlerror());
		return -1;
	}

	/* Get plugin init function */
	this->init = (clish_plugin_init_t *)dlsym(this->dlhan, CLISH_PLUGIN_INIT_NAME);
	if (!this->init) {
		fprintf(stderr, "Error: Can't get plugin %s init function: %s\n",
			this->file, dlerror());
		return -1;
	}

	/* Get plugin fini function */
	this->fini = (clish_plugin_fini_t *)dlsym(this->dlhan, CLISH_PLUGIN_FINI_NAME);

	/* Execute init function */
	if ((res = this->init(userdata, this)))
		fprintf(stderr, "Error: Plugin %s init retcode: %d\n",
			this->file, res);

	return res;
}

/*--------------------------------------------------------- */
void clish_plugin__set_name(clish_plugin_t *this, const char *name)
{
	lub_string_free(this->name);
	this->name = lub_string_dup(name);
}

/*--------------------------------------------------------- */
char *clish_plugin__get_name(const clish_plugin_t *this)
{
	return this->name;
}

/*--------------------------------------------------------- */
void clish_plugin__set_alias(clish_plugin_t *this, const char *alias)
{
	lub_string_free(this->alias);
	this->alias = lub_string_dup(alias);
}

/*--------------------------------------------------------- */
char *clish_plugin__get_alias(const clish_plugin_t *this)
{
	return this->alias;
}

/*--------------------------------------------------------- */
char *clish_plugin__get_pubname(const clish_plugin_t *this)
{
	return (this->alias ? this->alias : this->name);
}

/*--------------------------------------------------------- */
char *clish_plugin__get_file(const clish_plugin_t *this)
{
	return this->file;
}

/*--------------------------------------------------------- */
void clish_plugin__set_conf(clish_plugin_t *this, const char *conf)
{
	lub_string_free(this->conf);
	this->conf = lub_string_dup(conf);
}

/*--------------------------------------------------------- */
char *clish_plugin__get_conf(const clish_plugin_t *this)
{
	return this->conf;
}

/*--------------------------------------------------------- */
