/*
 * plugin.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "private.h"
#include "lub/string.h"
#include "lub/list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

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
	this->api = CLISH_SYM_API_SIMPLE;
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
int clish_sym__get_type(const clish_sym_t *this)
{
	return this->type;
}

/*--------------------------------------------------------- */
void clish_sym__set_api(clish_sym_t *this, clish_sym_api_e api)
{
	this->api = api;
}

/*--------------------------------------------------------- */
clish_sym_api_e clish_sym__get_api(const clish_sym_t *this)
{
	return this->api;
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
clish_plugin_t *clish_plugin_new(const char *name)
{
	clish_plugin_t *this;

	this = malloc(sizeof(*this));

	this->name = lub_string_dup(name);
	this->conf = NULL;
	this->alias = NULL;
	this->file = NULL;
	this->builtin_flag = BOOL_FALSE; /* The plugin is shared object by default */
	this->dlhan = NULL;
	/* Initialise the list of symbols */
	this->syms = lub_list_new(clish_sym_compare);
	/* Constructor and destructor */
	this->init = NULL;
	this->fini = NULL;
	/* Flags */
	this->rtld_global = BOOL_FALSE; /* The dlopen() use RTLD_LOCAL by default */

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

	lub_string_free(this->name);
	lub_string_free(this->alias);
	lub_string_free(this->file);
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
#ifdef HAVE_DLFCN_H
	if (this->dlhan)
		dlclose(this->dlhan);
#endif
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
clish_sym_t *clish_plugin_add_osym(clish_plugin_t *this,
	clish_hook_oaction_fn_t *func, const char *name)
{
	clish_sym_t *s;

	if (!(s = clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_FALSE)))
		return s;
	clish_sym__set_api(s, CLISH_SYM_API_STDOUT);

	return s;
}

/*--------------------------------------------------------- */
/* Add permanent symbol (can't be turned off by dry-run) */
clish_sym_t *clish_plugin_add_posym(clish_plugin_t *this,
	clish_hook_oaction_fn_t *func, const char *name)
{
	clish_sym_t *s;

	if (!(s = clish_plugin_add_generic(this, func,
		name, CLISH_SYM_TYPE_ACTION, BOOL_TRUE)))
		return s;
	clish_sym__set_api(s, CLISH_SYM_API_STDOUT);

	return s;
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
void clish_plugin_add_fini(clish_plugin_t *this,
	clish_plugin_fini_t *fini)
{
	this->fini = fini;
}

/*--------------------------------------------------------- */
clish_plugin_fini_t * clish_plugin_get_fini(clish_plugin_t *this)
{
	return this->fini;
}

/*--------------------------------------------------------- */
void clish_plugin_add_init(clish_plugin_t *this,
	clish_plugin_init_t *init)
{
	this->init = init;
}

/*--------------------------------------------------------- */
clish_plugin_init_t * clish_plugin_get_init(clish_plugin_t *this)
{
	return this->init;
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
static int clish_plugin_load_shared(clish_plugin_t *this)
{
#ifdef HAVE_DLFCN_H
	char *file = NULL; /* Plugin so file name */
	char *init_name = NULL; /* Init function name */
	int flag = RTLD_NOW;

	if (this->file) {
		file = lub_string_dup(this->file);
	} else {
		lub_string_cat(&file, "clish_plugin_");
		lub_string_cat(&file, this->name);
		lub_string_cat(&file, ".so");
	}

	/* Open dynamic library */
	if (clish_plugin__get_rtld_global(this))
		flag |= RTLD_GLOBAL;
	else
		flag |= RTLD_LOCAL;
	this->dlhan = dlopen(file, flag);
	lub_string_free(file);
	if (!this->dlhan) {
		fprintf(stderr, "Error: Can't open plugin \"%s\": %s\n",
			this->name, dlerror());
		return -1;
	}

	/* Get plugin init function */
	lub_string_cat(&init_name, CLISH_PLUGIN_INIT_NAME_PREFIX);
	lub_string_cat(&init_name, this->name);
	lub_string_cat(&init_name, CLISH_PLUGIN_INIT_NAME_SUFFIX);
	this->init = (clish_plugin_init_t *)dlsym(this->dlhan, init_name);
	lub_string_free(init_name);
	if (!this->init) {
		fprintf(stderr, "Error: Can't get plugin \"%s\" init function: %s\n",
			this->name, dlerror());
		return -1;
	}

	return 0;
#else /* HAVE_DLFCN_H */
	/* We have no any dl functions. */
	fprintf(stderr, "Error: Can't get plugin \"%s\" init function.\n",
		this->name);
	return -1;
#endif /* HAVE_DLFCN_H */
}

/*--------------------------------------------------------- */
int clish_plugin_load(clish_plugin_t *this, void *userdata)
{
	int res;

	if (!this)
		return -1;
	assert(this->name);

	/* Builtin plugins already have init function. */
	if (!this->builtin_flag) {
		if (clish_plugin_load_shared(this) < 0)
			return -1;
	}
	if (!this->init) {
		fprintf(stderr, "Error: PLUGIN %s has no init function\n",
			this->name);
		return -1;
	}
	/* Execute init function */
	if ((res = this->init(userdata, this)))
		fprintf(stderr, "Error: Plugin %s init retcode: %d\n",
			this->name, res);

	return res;
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
void clish_plugin__set_file(clish_plugin_t *this, const char *file)
{
	lub_string_free(this->file);
	this->file = lub_string_dup(file);
}

/*--------------------------------------------------------- */
char *clish_plugin__get_file(const clish_plugin_t *this)
{
	return this->file;
}

/*--------------------------------------------------------- */
void clish_plugin__set_builtin_flag(clish_plugin_t *this, bool_t builtin_flag)
{
	this->builtin_flag = builtin_flag;
}

/*--------------------------------------------------------- */
bool_t clish_plugin__get_builtin_flag(const clish_plugin_t *this)
{
	return this->builtin_flag;
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
void clish_plugin__set_rtld_global(clish_plugin_t *this, bool_t rtld_global)
{
	this->rtld_global = rtld_global;
}

/*--------------------------------------------------------- */
bool_t clish_plugin__get_rtld_global(const clish_plugin_t *this)
{
	return this->rtld_global;
}

/*--------------------------------------------------------- */
