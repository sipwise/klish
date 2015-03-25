/*
 * shell_plugin.c
 */
#include "private.h"
#include <assert.h>
#include <string.h>
#include <dlfcn.h>

#include "lub/string.h"
#include "lub/list.h"
#include "lub/bintree.h"
#include "clish/plugin.h"
#include "clish/view.h"

/*----------------------------------------------------------------------- */
clish_plugin_t * clish_shell_find_plugin(clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;

	assert(this);

	if (!name || !name[0])
		return NULL;
	/* Iterate elements */
	for(iter = lub_list__get_head(this->plugins);
		iter; iter = lub_list_node__get_next(iter)) {
		plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
		if (!strcmp(name, clish_plugin__get_name(plugin)))
			return plugin;
	}

	return NULL;
}

/*----------------------------------------------------------------------- */
clish_plugin_t * clish_shell_find_create_plugin(clish_shell_t *this,
	const char *name)
{
	clish_plugin_t *plugin;
	assert(this);

	if (!name || !name[0])
		return NULL;

	plugin = clish_shell_find_plugin(this, name);
	if (plugin)
		return plugin;
	plugin = clish_plugin_new(name);
	lub_list_add(this->plugins, plugin);

	return plugin;
}

/*----------------------------------------------------------------------- */
/* For all plugins:
 *  * dlopen(plugin)
 *  * dlsym(initialize function)
 *  * exec init functions to get all plugin syms
 */
int clish_shell_load_plugins(clish_shell_t *this)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;

	assert(this);

	/* Iterate elements */
	for(iter = lub_list__get_head(this->plugins);
		iter; iter = lub_list_node__get_next(iter)) {
		plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
		if (clish_plugin_load(plugin, (void *)this))
			return -1;
#ifdef DEBUG
		clish_plugin_dump(plugin);
#endif
	}

	return 0;
}

/*----------------------------------------------------------------------- */
/* Iterate plugins to find symbol by name.
 * The symbol name can be simple or with namespace:
 * mysym@plugin1
 * The symbols with suffix will be resolved using specified plugin only.
 */
static clish_sym_t *plugins_find_sym(clish_shell_t *this, const char *name, int type)
{
	lub_list_node_t *iter;
	clish_plugin_t *plugin;
	clish_sym_t *sym = NULL;
	/* To parse command name */
	char *saveptr = NULL;
	const char *delim = "@";
	char *plugin_name = NULL;
	char *cmdn = NULL;
	char *str = lub_string_dup(name);

	assert(this);

	/* Parse name to get sym name and optional plugin name */
	cmdn = strtok_r(str, delim, &saveptr);
	if (!cmdn) {
		lub_string_free(str);
		return NULL;
	}
	plugin_name = strtok_r(NULL, delim, &saveptr);

	if (plugin_name) {
		/* Search for symbol in specified namespace */
		/* Iterate elements */
		for(iter = lub_list__get_head(this->plugins);
			iter; iter = lub_list_node__get_next(iter)) {
			plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
			if (strcmp(clish_plugin__get_pubname(plugin), plugin_name))
				continue;
			if ((sym = clish_plugin_get_sym(plugin, cmdn, type)))
				break;
		}
	} else {
		/* Iterate all plugins */
		for(iter = lub_list__get_head(this->plugins);
			iter; iter = lub_list_node__get_next(iter)) {
			plugin = (clish_plugin_t *)lub_list_node__get_data(iter);
			if ((sym = clish_plugin_get_sym(plugin, cmdn, type)))
				break;
		}
	}
	lub_string_free(str);

	return sym;
}

/*--------------------------------------------------------- */
/* Find symbol by name in the list of unresolved symbols */
clish_sym_t *clish_shell_find_sym(clish_shell_t *this, const char *name, int type)
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
		if (res > 0) /* No chance to find name */
			break;
	}

	return NULL;
}

/*----------------------------------------------------------------------- */
/* Add symbol to the table of unresolved symbols */
clish_sym_t *clish_shell_add_sym(clish_shell_t *this,
	void *func, const char *name, int type)
{
	clish_sym_t *sym = NULL;

	if (!name)
		return NULL;
	if ((sym = clish_shell_find_sym(this, name, type)))
		return sym;
	if (!(sym = clish_sym_new(name, func, type)))
		return NULL;
	lub_list_add(this->syms, sym);

	return sym;
}

/*----------------------------------------------------------------------- */
clish_sym_t *clish_shell_add_unresolved_sym(clish_shell_t *this,
	const char *name, int type)
{
	return clish_shell_add_sym(this, NULL, name, type);
}

/*----------------------------------------------------------------------- */
/* Link one unresolved symbol.
 * sym - unresolved symbol
 * Returns 0 if the symbol was succesfully resolved
 */
static int link_unresolved_sym(clish_shell_t *this, clish_sym_t *sym)
{
	clish_sym_t *plugin_sym;
	char *sym_name = NULL;
	int sym_type;

	if (clish_sym__get_func(sym)) /* Don't relink non-null fn */
		return 0;
	sym_name = clish_sym__get_name(sym);
	sym_type = clish_sym__get_type(sym);
	plugin_sym = plugins_find_sym(this, sym_name, sym_type);
	if (!plugin_sym) {
		fprintf(stderr, "Error: Can't resolve symbol %s.\n",
			sym_name);
		return -1;
	}
	/* Copy symbol attributes */
	clish_sym_clone(sym, plugin_sym);

	return 0;
}

/*----------------------------------------------------------------------- */
/* Link unresolved symbols */
int clish_shell_link_plugins(clish_shell_t *this)
{
	clish_sym_t *sym;
	lub_list_node_t *iter;

	/* Iterate elements */
	for(iter = lub_list__get_head(this->syms);
		iter; iter = lub_list_node__get_next(iter)) {
		sym = (clish_sym_t *)lub_list_node__get_data(iter);
		if (link_unresolved_sym(this, sym) < 0)
			return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------- */
/* Get hook sym */
clish_sym_t *clish_shell_get_hook(const clish_shell_t *this, int type)
{
	return this->hooks[type];
}
