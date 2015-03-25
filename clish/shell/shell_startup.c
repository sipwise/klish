/*
 * shell_startup.c
 */
#include "private.h"
#include <assert.h>

#include "lub/string.h"

/* Default hooks */
const char* clish_plugin_default_hook[] = {
	NULL,
	"clish_script@clish",
	NULL,
	"clish_hook_config@clish",
	"clish_hook_log@clish"
};

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
	clish_view_t *view;

	assert(this);
	assert(this->startup);
	/* Search for the view */
	view = clish_shell_find_view(this, viewname);
	assert(view);
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



/*-------------------------------------------------------- */
/* Resolve PTYPE for given PARAM.
 */
static clish_ptype_t * resolve_ptype(clish_shell_t *this, clish_param_t *param)
{
	clish_ptype_t *ptype = NULL;

	if (!this || !param)
		return NULL;

	/* Resolve PARAM's PTYPE */
	ptype = clish_shell_find_ptype(this, clish_param__get_ptype_name(param));
	if (!ptype) {
		fprintf(stderr, "Error: Unresolved PTYPE \"%s\" in PARAM \"%s\"\n",
			clish_param__get_ptype_name(param),
			clish_param__get_name(param));
		return NULL;
	}
	clish_param__set_ptype(param, ptype);
	clish_param__set_ptype_name(param, NULL); /* Free some memory */

	return ptype;
}

/*-------------------------------------------------------- */
/* Static recursive function to iterate parameters. Logically it's the
 * part of clish_shell_prepare() function.
 */
static int iterate_paramv(clish_shell_t *this, clish_paramv_t *paramv,
	clish_hook_access_fn_t *access_fn)
{
	int i = 0;
	clish_param_t *param;

	while((param = clish_paramv__get_param(paramv, i))) {
		clish_paramv_t *nested_paramv;

		/* Resolve PARAM's PTYPE */
		if (!resolve_ptype(this, param))
			return -1;

		/* Check access for PARAM */
		if (access_fn && clish_param__get_access(param) &&
			access_fn(this, clish_param__get_access(param))) {
#ifdef DEBUG
			fprintf(stderr, "Warning: Access denied. Remove PARAM \"%s\"\n",
				clish_param__get_name(param));
#endif
			if (clish_paramv_remove(paramv, i) < 0) {
				fprintf(stderr, "Error: Some system problem\n");
				return -1;
			}
			clish_param_delete(param);
			continue; /* Don't increment index */
		}
		/* Recursive iterate nested PARAMs */
		nested_paramv = clish_param__get_paramv(param);
		if (iterate_paramv(this, nested_paramv, access_fn) < 0)
			return -1;
		i++;
	}

	return 0;
}

/*-------------------------------------------------------- */
/* This function prepares schema for execution. It loads
 * plugins, link unresolved symbols, then iterates all the
 * objects and link them to each other, check access
 * permissions. Don't execute clish_shell_startup() without this
 * function.
 */
int clish_shell_prepare(clish_shell_t *this)
{
	clish_command_t *cmd;
	clish_view_t *view;
	clish_nspace_t *nspace;
	lub_bintree_t *view_tree, *cmd_tree;
	lub_list_t *nspace_tree;
	lub_bintree_iterator_t cmd_iter, view_iter;
	lub_list_node_t *nspace_iter;
	clish_hook_access_fn_t *access_fn = NULL;
	clish_paramv_t *paramv;
	int i = 0;

	/* Add statically linked plugins */
	while (clish_plugin_builtin_list[i].name) {
		clish_plugin_t *plugin;
		plugin = clish_shell_find_create_plugin(this,
			clish_plugin_builtin_list[i].name);
		clish_plugin_add_init(plugin,
			clish_plugin_builtin_list[i].init);
		clish_plugin__set_builtin_flag(plugin, BOOL_TRUE);
		i++;
	}

	/* Add default plugin to the list of plugins */
	if (this->default_plugin) {
		clish_shell_find_create_plugin(this, "clish");
		/* Default hooks */
		for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
			if (this->hooks_use[i])
				continue;
			if (!clish_plugin_default_hook[i])
				continue;
			clish_sym__set_name(this->hooks[i],
				clish_plugin_default_hook[i]);
		}
	}
	/* Add default syms to unresolved table */
	for (i = 0; i < CLISH_SYM_TYPE_MAX; i++) {
		if (clish_sym__get_name(this->hooks[i]))
			lub_list_add(this->syms, this->hooks[i]);
	}

	/* Load plugins and link symbols */
	if (clish_shell_load_plugins(this) < 0)
		return -1;
	if (clish_shell_link_plugins(this) < 0)
		return -1;

	access_fn = clish_sym__get_func(clish_shell_get_hook(this, CLISH_SYM_TYPE_ACCESS));

	/* Iterate the VIEWs */
	view_tree = &this->view_tree;
	view = lub_bintree_findfirst(view_tree);
	for (lub_bintree_iterator_init(&view_iter, view_tree, view);
		view; view = lub_bintree_iterator_next(&view_iter)) {
		/* Check access rights for the VIEW */
		if (access_fn && clish_view__get_access(view) &&
			access_fn(this, clish_view__get_access(view))) {
#ifdef DEBUG
			fprintf(stderr, "Warning: Access denied. Remove VIEW \"%s\"\n",
				clish_view__get_name(view));
#endif
			lub_bintree_remove(view_tree, view);
			clish_view_delete(view);
			continue;
		}

		/* Iterate the NAMESPACEs */
		nspace_tree = clish_view__get_nspace_tree(view);
		nspace_iter = lub_list__get_head(nspace_tree);
		while(nspace_iter) {
			clish_view_t *ref_view;
			lub_list_node_t *old_nspace_iter;
			nspace = (clish_nspace_t *)lub_list_node__get_data(nspace_iter);
			old_nspace_iter = nspace_iter;
			nspace_iter = lub_list_node__get_next(nspace_iter);
			/* Resolve NAMESPACEs and remove unresolved ones */
			ref_view = clish_shell_find_view(this, clish_nspace__get_view_name(nspace));
			if (!ref_view) {
#ifdef DEBUG
				fprintf(stderr, "Warning: Remove unresolved NAMESPACE \"%s\" from \"%s\" VIEW\n",
					clish_nspace__get_view_name(nspace), clish_view__get_name(view));
#endif
				lub_list_del(nspace_tree, old_nspace_iter);
				lub_list_node_free(old_nspace_iter);
				clish_nspace_delete(nspace);
				continue;
			}
			clish_nspace__set_view(nspace, ref_view);
			clish_nspace__set_view_name(nspace, NULL); /* Free some memory */
			/* Check access rights for the NAMESPACE */
			if (access_fn && (
				/* Check NAMESPASE owned access */
				(clish_nspace__get_access(nspace) && access_fn(this, clish_nspace__get_access(nspace)))
				||
				/* Check referenced VIEW's access */
				(clish_view__get_access(ref_view) && access_fn(this, clish_view__get_access(ref_view)))
				)) {
#ifdef DEBUG
				fprintf(stderr, "Warning: Access denied. Remove NAMESPACE \"%s\" from \"%s\" VIEW\n",
					clish_nspace__get_view_name(nspace), clish_view__get_name(view));
#endif
				lub_list_del(nspace_tree, old_nspace_iter);
				lub_list_node_free(old_nspace_iter);
				clish_nspace_delete(nspace);
				continue;
			}
		}

		/* Iterate the COMMANDs */
		cmd_tree = clish_view__get_command_tree(view);
		cmd = lub_bintree_findfirst(cmd_tree);
		for (lub_bintree_iterator_init(&cmd_iter, cmd_tree, cmd);
			cmd; cmd = lub_bintree_iterator_next(&cmd_iter)) {
			int cmd_is_alias = clish_command__get_alias(cmd)?1:0;
			clish_param_t *args = NULL;

			/* Check access rights for the COMMAND */
			if (access_fn && clish_command__get_access(cmd) &&
				access_fn(this, clish_command__get_access(cmd))) {
#ifdef DEBUG
				fprintf(stderr, "Warning: Access denied. Remove COMMAND \"%s\" from VIEW \"%s\"\n",
					clish_command__get_name(cmd), clish_view__get_name(view));
#endif
				lub_bintree_remove(cmd_tree, cmd);
				clish_command_delete(cmd);
				continue;
			}

			/* Resolve command aliases */
			if (cmd_is_alias) {
				clish_view_t *aview;
				clish_command_t *cmdref;
				const char *alias_view = clish_command__get_alias_view(cmd);
				if (!alias_view)
					aview = clish_command__get_pview(cmd);
				else
					aview = clish_shell_find_view(this, alias_view);
				if (!aview /* Removed or broken VIEW */
					||
					/* Removed or broken referenced COMMAND */
					!(cmdref = clish_view_find_command(aview, clish_command__get_alias(cmd), BOOL_FALSE))
					) {
#ifdef DEBUG
					fprintf(stderr, "Warning: Remove unresolved link \"%s\" from \"%s\" VIEW\n",
						clish_command__get_name(cmd), clish_view__get_name(view));
#endif
					lub_bintree_remove(cmd_tree, cmd);
					clish_command_delete(cmd);
					continue;
					/*fprintf(stderr, CLISH_XML_ERROR_STR"Broken VIEW for alias \"%s\"\n",
						clish_command__get_name(cmd));
					return -1; */
					/*fprintf(stderr, CLISH_XML_ERROR_STR"Broken alias \"%s\"\n",
						clish_command__get_name(cmd));
					return -1; */
				}
				if (!clish_command_alias_to_link(cmd, cmdref)) {
					fprintf(stderr, CLISH_XML_ERROR_STR"Something wrong with alias \"%s\"\n",
						clish_command__get_name(cmd));
					return -1;
				}
				/* Check access rights for newly constructed COMMAND.
				   Now the link has access filed from referenced command.
				 */
				if (access_fn && clish_command__get_access(cmd) &&
					access_fn(this, clish_command__get_access(cmd))) {
#ifdef DEBUG
					fprintf(stderr, "Warning: Access denied. Remove COMMAND \"%s\" from VIEW \"%s\"\n",
						clish_command__get_name(cmd), clish_view__get_name(view));
#endif
					lub_bintree_remove(cmd_tree, cmd);
					clish_command_delete(cmd);
					continue;
				}
			}
			if (cmd_is_alias) /* Don't duplicate paramv processing for aliases */
				continue;
			/* Iterate PARAMeters */
			paramv = clish_command__get_paramv(cmd);
			if (iterate_paramv(this, paramv, access_fn) < 0)
				return -1;
			/* Resolve PTYPE for args */
			if ((args = clish_command__get_args(cmd))) {
				if (!resolve_ptype(this, args))
					return -1;
			}
		}
	}

	return 0;
}

/*----------------------------------------------------------- */
