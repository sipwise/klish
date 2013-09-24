/*
 * ------------------------------------------------------
 * shell_xml.c
 *
 * This file implements the means to read an XML encoded file and populate the
 * CLI tree based on the contents.
 * ------------------------------------------------------
 */
#include "private.h"
#include "xmlapi.h"
#include "lub/string.h"
#include "lub/ctype.h"
#include "lub/system.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

typedef void (PROCESS_FN) (clish_shell_t * instance,
	clish_xmlnode_t * element, void *parent);

/* Define a control block for handling the decode of an XML file */
typedef struct clish_xml_cb_s clish_xml_cb_t;
struct clish_xml_cb_s {
	const char *element;
	PROCESS_FN *handler;
};

/* forward declare the handler functions */
static PROCESS_FN
	process_clish_module,
	process_startup,
	process_view,
	process_command,
	process_param,
	process_action,
	process_ptype,
	process_overview,
	process_detail,
	process_namespace,
	process_config,
	process_var,
	process_wdog,
	process_hotkey;

static clish_xml_cb_t xml_elements[] = {
	{"CLISH_MODULE", process_clish_module},
	{"STARTUP", process_startup},
	{"VIEW", process_view},
	{"COMMAND", process_command},
	{"PARAM", process_param},
	{"ACTION", process_action},
	{"PTYPE", process_ptype},
	{"OVERVIEW", process_overview},
	{"DETAIL", process_detail},
	{"NAMESPACE", process_namespace},
	{"CONFIG", process_config},
	{"VAR", process_var},
	{"WATCHDOG", process_wdog},
	{"HOTKEY", process_hotkey},
	{NULL, NULL}
};

/*
 * if CLISH_PATH is unset in the environment then this is the value used. 
 */
const char *default_path = "/etc/clish;~/.clish";

/*-------------------------------------------------------- */
void clish_shell_load_scheme(clish_shell_t *this, const char *xml_path)
{
	const char *path = xml_path;
	char *buffer;
	char *dirname;
	char *saveptr;

	/* use the default path */
	if (!path)
		path = default_path;
	/* take a copy of the path */
	buffer = lub_system_tilde_expand(path);

	/* now loop though each directory */
	for (dirname = strtok_r(buffer, ";", &saveptr);
		dirname; dirname = strtok_r(NULL, ";", &saveptr)) {
		DIR *dir;
		struct dirent *entry;

		/* search this directory for any XML files */
		dir = opendir(dirname);
		if (NULL == dir) {
#ifdef DEBUG
			tinyrl_printf(this->tinyrl,
				"*** Failed to open '%s' directory\n",
				dirname);
#endif
			continue;
		}
		for (entry = readdir(dir); entry; entry = readdir(dir)) {
			const char *extension = strrchr(entry->d_name, '.');
			/* check the filename */
			if ((NULL != extension) &&
				(0 == strcmp(".xml", extension))) {
				char *filename = NULL;

				/* build the filename */
				lub_string_cat(&filename, dirname);
				lub_string_cat(&filename, "/");
				lub_string_cat(&filename, entry->d_name);

#ifdef DEBUG
				fprintf(stderr, "Parse XML-file: %s\n", filename);
#endif
				/* load this file */
				(void)clish_shell_xml_read(this, filename);

				/* release the resource */
				lub_string_free(filename);
			}
		}
		/* all done for this directory */
		closedir(dir);
	}
	/* tidy up */
	lub_string_free(buffer);
#ifdef DEBUG
	clish_shell_dump(this);
#endif
}

/*
 * ------------------------------------------------------
 * This function reads an element from the XML stream and processes it.
 * ------------------------------------------------------
 */
static void process_node(clish_shell_t * shell, clish_xmlnode_t * node, void *parent)
{
	switch (clish_xmlnode_get_type(node)) {
	case CLISH_XMLNODE_ELM: {
			clish_xml_cb_t * cb;
			char name[128];
			unsigned int namelen = sizeof(name);
			if (clish_xmlnode_get_name(node, name, &namelen) == 0) {
				for (cb = &xml_elements[0]; cb->element; cb++) {
					if (0 == strcmp(name, cb->element)) {
#ifdef DEBUG
						fprintf(stderr, "NODE:");
						clish_xmlnode_print(node, stderr);
						fprintf(stderr, "\n");
#endif
						/* process the elements at this level */
						cb->handler(shell, node, parent);
						break;
					}
				}
			}
			break;
		}
	case CLISH_XMLNODE_DOC:
	case CLISH_XMLNODE_TEXT:
	case CLISH_XMLNODE_ATTR:
	case CLISH_XMLNODE_PI:
	case CLISH_XMLNODE_COMMENT:
	case CLISH_XMLNODE_DECL:
	case CLISH_XMLNODE_UNKNOWN:
	default:
		break;
	}
}

/* ------------------------------------------------------ */
static void process_children(clish_shell_t * shell,
	clish_xmlnode_t * element, void *parent)
{
	clish_xmlnode_t *node = NULL;

	while ((node = clish_xmlnode_next_child(element, node)) != NULL) {
		/* Now deal with all the contained elements */
		process_node(shell, node, parent);
	}
}

/* ------------------------------------------------------ */
static void
process_clish_module(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	// create the global view
	if (!shell->global)
		shell->global = clish_shell_find_create_view(shell,
			"global", "");
	process_children(shell, element, shell->global);
}

/* ------------------------------------------------------ */
static void process_view(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *view;
	int allowed = 1;

	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *prompt = clish_xmlnode_fetch_attr(element, "prompt");
	char *depth = clish_xmlnode_fetch_attr(element, "depth");
	char *restore = clish_xmlnode_fetch_attr(element, "restore");
	char *access = clish_xmlnode_fetch_attr(element, "access");

	/* Check permissions */
	if (access) {
		allowed = 0;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		goto process_view_end;

	assert(name);

	/* re-use a view if it already exists */
	view = clish_shell_find_create_view(shell, name, prompt);

	if (depth && (lub_ctype_isdigit(*depth))) {
		unsigned res = atoi(depth);
		clish_view__set_depth(view, res);
	}

	if (restore) {
		if (!lub_string_nocasecmp(restore, "depth"))
			clish_view__set_restore(view, CLISH_RESTORE_DEPTH);
		else if (!lub_string_nocasecmp(restore, "view"))
			clish_view__set_restore(view, CLISH_RESTORE_VIEW);
		else
			clish_view__set_restore(view, CLISH_RESTORE_NONE);
	}

	process_children(shell, element, view);

process_view_end:
	clish_xml_release(name);
	clish_xml_release(prompt);
	clish_xml_release(depth);
	clish_xml_release(restore);
	clish_xml_release(access);
}

/* ------------------------------------------------------ */
static void process_ptype(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	clish_ptype_t *ptype;

	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *help = clish_xmlnode_fetch_attr(element, "help");
	char *pattern = clish_xmlnode_fetch_attr(element, "pattern");
	char *method_name = clish_xmlnode_fetch_attr(element, "method");
	char *preprocess_name =	clish_xmlnode_fetch_attr(element, "preprocess");

	assert(name);
	assert(pattern);

	method = clish_ptype_method_resolve(method_name);

	preprocess = clish_ptype_preprocess_resolve(preprocess_name);
	ptype = clish_shell_find_create_ptype(shell,
		name, help, pattern, method, preprocess);

	assert(ptype);

	clish_xml_release(name);
	clish_xml_release(help);
	clish_xml_release(pattern);
	clish_xml_release(method_name);
	clish_xml_release(preprocess_name);
}

/* ------------------------------------------------------ */
static void
process_overview(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	char *content = NULL;
	unsigned int content_len = 2048;
	int result;

	/*
	 * the code below faithfully assume that we'll be able fully store
	 * the content of the node. If it's really, really big, we may have
	 * an issue (but then, if it's that big, how the hell does it
	 * already fits in allocated memory?)
	 * Ergo, it -should- be safe.
	 */
	do {
		content = (char*)realloc(content, content_len);
		result = clish_xmlnode_get_content(element, content,
			&content_len);
	} while (result == -E2BIG);

	if (result == 0 && content) {
		/* set the overview text for this view */
		assert(NULL == shell->overview);
		/* store the overview */
		shell->overview = lub_string_dup(content);
	}

	if (content)
		free(content);
}

/* ------------------------------------------------------ */
static void
process_command(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	clish_command_t *old;
	char *alias_name = NULL;
	clish_view_t *alias_view = NULL;
	int allowed = 1;

	char *access = clish_xmlnode_fetch_attr(element, "access");
	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *help = clish_xmlnode_fetch_attr(element, "help");
	char *view = clish_xmlnode_fetch_attr(element, "view");
	char *viewid = clish_xmlnode_fetch_attr(element, "viewid");
	char *escape_chars = clish_xmlnode_fetch_attr(element, "escape_chars");
	char *args_name = clish_xmlnode_fetch_attr(element, "args");
	char *args_help = clish_xmlnode_fetch_attr(element, "args_help");
	char *lock = clish_xmlnode_fetch_attr(element, "lock");
	char *interrupt = clish_xmlnode_fetch_attr(element, "interrupt");
	char *ref = clish_xmlnode_fetch_attr(element, "ref");

	/* Check permissions */
	if (access) {
		allowed = 0;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		goto process_command_end;

	assert(name);

	/* check this command doesn't already exist */
	old = clish_view_find_command(v, name, BOOL_FALSE);
	if (old) {
		/* flag the duplication then ignore further definition */
		printf("DUPLICATE COMMAND: %s\n",
		       clish_command__get_name(old));
		goto process_command_end;
	}

	assert(help);

	/* Reference 'ref' field */
	if (ref) {
		char *saveptr;
		const char *delim = "@";
		char *view_name = NULL;
		char *cmdn = NULL;
		char *str = lub_string_dup(ref);

		cmdn = strtok_r(str, delim, &saveptr);
		if (!cmdn) {
			printf("EMPTY REFERENCE COMMAND: %s\n", name);
			lub_string_free(str);
			goto process_command_end;
		}
		alias_name = lub_string_dup(cmdn);
		view_name = strtok_r(NULL, delim, &saveptr);
		if (!view_name)
			alias_view = v;
		else
			alias_view = clish_shell_find_create_view(shell,
				view_name, NULL);
		lub_string_free(str);
	}

	/* create a command */
	cmd = clish_view_new_command(v, name, help);
	assert(cmd);
	clish_command__set_pview(cmd, v);

	/* define some specialist escape characters */
	if (escape_chars)
		clish_command__set_escape_chars(cmd, escape_chars);

	if (args_name) {
		/* define a "rest of line" argument */
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		assert(args_help);
		tmp = clish_shell_find_ptype(shell, "internal_ARGS");
		assert(tmp);
		param = clish_param_new(args_name, args_help, tmp);
		clish_command__set_args(cmd, param);
	}

	/* Define the view which this command changes to */
	if (view)
		clish_command__set_view(cmd, view);

	/* define the view id which this command changes to */
	if (viewid)
		clish_command__set_viewid(cmd, viewid);

	/* lock field */
	if (lock && lub_string_nocasecmp(lock, "false") == 0)
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);

	/* interrupt field */
	if (interrupt && lub_string_nocasecmp(interrupt, "true") == 0)
		clish_command__set_interrupt(cmd, BOOL_TRUE);
	else
		clish_command__set_interrupt(cmd, BOOL_FALSE);

	/* Set alias */
	if (alias_name) {
		assert(!((alias_view == v) && (!strcmp(alias_name, name))));
		clish_command__set_alias(cmd, alias_name);
		assert(alias_view);
		clish_command__set_alias_view(cmd, alias_view);
		lub_string_free(alias_name);
	}

	process_children(shell, element, cmd);

process_command_end:
	clish_xml_release(access);
	clish_xml_release(name);
	clish_xml_release(help);
	clish_xml_release(view);
	clish_xml_release(viewid);
	clish_xml_release(escape_chars);
	clish_xml_release(args_name);
	clish_xml_release(args_help);
	clish_xml_release(lock);
	clish_xml_release(interrupt);
	clish_xml_release(ref);
}

/* ------------------------------------------------------ */
static void
process_startup(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;

	char *view = clish_xmlnode_fetch_attr(element, "view");
	char *viewid = clish_xmlnode_fetch_attr(element, "viewid");
	char *default_shebang =
		clish_xmlnode_fetch_attr(element, "default_shebang");
	char *timeout = clish_xmlnode_fetch_attr(element, "timeout");
	char *lock = clish_xmlnode_fetch_attr(element, "lock");
	char *interrupt = clish_xmlnode_fetch_attr(element, "interrupt");

	assert(!shell->startup);

	assert(view);

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "startup", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* define the view which this command changes to */
	clish_command__set_view(cmd, view);

	/* define the view id which this command changes to */
	if (viewid)
		clish_command__set_viewid(cmd, viewid);

	if (default_shebang)
		clish_shell__set_default_shebang(shell, default_shebang);

	if (timeout)
		clish_shell__set_timeout(shell, atoi(timeout));

	/* lock field */
	if (lock && lub_string_nocasecmp(lock, "false") == 0)
		clish_command__set_lock(cmd, BOOL_FALSE);
	else
		clish_command__set_lock(cmd, BOOL_TRUE);

	/* interrupt field */
	if (interrupt && lub_string_nocasecmp(interrupt, "true") == 0)
		clish_command__set_interrupt(cmd, BOOL_TRUE);
	else
		clish_command__set_interrupt(cmd, BOOL_FALSE);

	/* remember this command */
	shell->startup = cmd;

	clish_xml_release(view);
	clish_xml_release(viewid);
	clish_xml_release(default_shebang);
	clish_xml_release(timeout);
	clish_xml_release(lock);
	clish_xml_release(interrupt);

	process_children(shell, element, cmd);
}

/* ------------------------------------------------------ */
static void
process_param(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = NULL;
	clish_param_t *p_param = NULL;
	clish_xmlnode_t *pelement;
	char *pname;

	pelement = clish_xmlnode_parent(element);
	pname = clish_xmlnode_get_all_name(pelement);

	if (pname && lub_string_nocasecmp(pname, "PARAM") == 0)
		p_param = (clish_param_t *)parent;
	else
		cmd = (clish_command_t *)parent;

	if (pname)
		free(pname);

	if (cmd || p_param) {
		char *name = clish_xmlnode_fetch_attr(element, "name");
		char *help = clish_xmlnode_fetch_attr(element, "help");
		char *ptype = clish_xmlnode_fetch_attr(element, "ptype");
		char *prefix = clish_xmlnode_fetch_attr(element, "prefix");
		char *defval = clish_xmlnode_fetch_attr(element, "default");
		char *mode = clish_xmlnode_fetch_attr(element, "mode");
		char *optional = clish_xmlnode_fetch_attr(element, "optional");
		char *order = clish_xmlnode_fetch_attr(element, "order");
		char *value = clish_xmlnode_fetch_attr(element, "value");
		char *hidden = clish_xmlnode_fetch_attr(element, "hidden");
		char *test = clish_xmlnode_fetch_attr(element, "test");
		char *completion = clish_xmlnode_fetch_attr(element, "completion");
		clish_param_t *param;
		clish_ptype_t *tmp = NULL;

		assert((!cmd) || (cmd != shell->startup));

		/* create a command */
		assert(name);
		assert(help);
		assert(ptype);

		if (*ptype) {
			tmp = clish_shell_find_create_ptype(shell, ptype,
				NULL, NULL,
				CLISH_PTYPE_REGEXP,
				CLISH_PTYPE_NONE);
			assert(tmp);
		}
		param = clish_param_new(name, help, tmp);

		/* If prefix is set clish will emulate old optional
		 * command syntax over newer optional command mechanism.
		 * It will create nested PARAM.
		 */
		if (prefix) {
			const char *ptype_name = "__SUBCOMMAND";
			clish_param_t *opt_param = NULL;

			/* Create a ptype for prefix-named subcommand that
			 * will contain the nested optional parameter. The
			 * name of ptype is hardcoded. It's not good but
			 * it's only the service ptype.
			 */
			tmp = (clish_ptype_t *)lub_bintree_find(
				&shell->ptype_tree, ptype_name);
			if (!tmp)
				tmp = clish_shell_find_create_ptype(shell,
					ptype_name, "Option", "[^\\\\]+",
					CLISH_PTYPE_REGEXP, CLISH_PTYPE_NONE);
			assert(tmp);
			opt_param = clish_param_new(prefix, help, tmp);
			clish_param__set_mode(opt_param,
				CLISH_PARAM_SUBCOMMAND);
			clish_param__set_optional(opt_param, BOOL_TRUE);

			if (test)
				clish_param__set_test(opt_param, test);

			/* add the parameter to the command */
			if (cmd)
				clish_command_insert_param(cmd, opt_param);
			/* add the parameter to the param */
			if (p_param)
				clish_param_insert_param(p_param, opt_param);
			/* Unset cmd and set parent param to opt_param */
			cmd = NULL;
			p_param = opt_param;
		}

		if (defval)
			clish_param__set_default(param, defval);

		if (hidden && lub_string_nocasecmp(hidden, "true") == 0)
			clish_param__set_hidden(param, BOOL_TRUE);
		else
			clish_param__set_hidden(param, BOOL_FALSE);

		if (mode) {
			if (lub_string_nocasecmp(mode, "switch") == 0) {
				clish_param__set_mode(param,
					CLISH_PARAM_SWITCH);
				/* Force hidden attribute */
				clish_param__set_hidden(param, BOOL_TRUE);
			} else if (lub_string_nocasecmp(mode, "subcommand") == 0)
				clish_param__set_mode(param,
					CLISH_PARAM_SUBCOMMAND);
			else
				clish_param__set_mode(param,
					CLISH_PARAM_COMMON);
		}

		if (optional && lub_string_nocasecmp(optional, "true") == 0)
			clish_param__set_optional(param, BOOL_TRUE);
		else
			clish_param__set_optional(param, BOOL_FALSE);

		if (order && lub_string_nocasecmp(order, "true") == 0)
			clish_param__set_order(param, BOOL_TRUE);
		else
			clish_param__set_order(param, BOOL_FALSE);

		if (value) {
			clish_param__set_value(param, value);
			/* Force mode to subcommand */
			clish_param__set_mode(param,
				CLISH_PARAM_SUBCOMMAND);
		}

		if (test && !prefix)
			clish_param__set_test(param, test);

		if (completion)
			clish_param__set_completion(param, completion);

		/* add the parameter to the command */
		if (cmd)
			clish_command_insert_param(cmd, param);

		/* add the parameter to the param */
		if (p_param)
			clish_param_insert_param(p_param, param);

		clish_xml_release(name);
		clish_xml_release(help);
		clish_xml_release(ptype);
		clish_xml_release(prefix);
		clish_xml_release(defval);
		clish_xml_release(mode);
		clish_xml_release(optional);
		clish_xml_release(order);
		clish_xml_release(value);
		clish_xml_release(hidden);
		clish_xml_release(test);
		clish_xml_release(completion);

		process_children(shell, element, param);
	}
}

/* ------------------------------------------------------ */
static void
process_action(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_action_t *action = NULL;
	char *builtin = clish_xmlnode_fetch_attr(element, "builtin");
	char *shebang = clish_xmlnode_fetch_attr(element, "shebang");
	clish_xmlnode_t *pelement = clish_xmlnode_parent(element);
	char *pname = clish_xmlnode_get_all_name(pelement);
	char *text;

	if (pname && lub_string_nocasecmp(pname, "VAR") == 0)
		action = clish_var__get_action((clish_var_t *)parent);
	else
		action = clish_command__get_action((clish_command_t *)parent);
	assert(action);

	if (pname)
		free(pname);

	text = clish_xmlnode_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_action__set_script(action, text);
	}
	if (text)
		free(text);

	if (builtin)
		clish_action__set_builtin(action, builtin);
	if (shebang)
		clish_action__set_shebang(action, shebang);

	clish_xml_release(builtin);
	clish_xml_release(shebang);
}

/* ------------------------------------------------------ */
static void
process_detail(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *) parent;

	/* read the following text element */
	char *text = clish_xmlnode_get_all_content(element);

	if (text && *text) {
		/* store the action */
		clish_command__set_detail(cmd, text);
	}

	if (text)
		free(text);
}

/* ------------------------------------------------------ */
static void
process_namespace(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_nspace_t *nspace = NULL;

	char *view = clish_xmlnode_fetch_attr(element, "ref");
	char *prefix = clish_xmlnode_fetch_attr(element, "prefix");
	char *prefix_help = clish_xmlnode_fetch_attr(element, "prefix_help");
	char *help = clish_xmlnode_fetch_attr(element, "help");
	char *completion = clish_xmlnode_fetch_attr(element, "completion");
	char *context_help = clish_xmlnode_fetch_attr(element, "context_help");
	char *inherit = clish_xmlnode_fetch_attr(element, "inherit");
	char *access = clish_xmlnode_fetch_attr(element, "access");

	int allowed = 1;

	if (access) {
		allowed = 0;
		if (shell->client_hooks->access_fn)
			allowed = shell->client_hooks->access_fn(shell, access);
	}
	if (!allowed)
		goto process_namespace_end;

	assert(view);
	clish_view_t *ref_view = clish_shell_find_create_view(shell,
		view, NULL);
	assert(ref_view);

	/* Don't include itself without prefix */
	if ((ref_view == v) && !prefix)
		goto process_namespace_end;

	nspace = clish_nspace_new(ref_view);
	assert(nspace);
	clish_view_insert_nspace(v, nspace);

	if (prefix) {
		clish_nspace__set_prefix(nspace, prefix);
		if (prefix_help)
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				prefix_help);
		else
			clish_nspace_create_prefix_cmd(nspace,
				"prefix",
				"Prefix for the imported commands.");
	}

	if (help && lub_string_nocasecmp(help, "true") == 0)
		clish_nspace__set_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_help(nspace, BOOL_FALSE);

	if (completion && lub_string_nocasecmp(completion, "false") == 0)
		clish_nspace__set_completion(nspace, BOOL_FALSE);
	else
		clish_nspace__set_completion(nspace, BOOL_TRUE);

	if (context_help && lub_string_nocasecmp(context_help, "true") == 0)
		clish_nspace__set_context_help(nspace, BOOL_TRUE);
	else
		clish_nspace__set_context_help(nspace, BOOL_FALSE);

	if (inherit && lub_string_nocasecmp(inherit, "false") == 0)
		clish_nspace__set_inherit(nspace, BOOL_FALSE);
	else
		clish_nspace__set_inherit(nspace, BOOL_TRUE);

process_namespace_end:
	clish_xml_release(view);
	clish_xml_release(prefix);
	clish_xml_release(prefix_help);
	clish_xml_release(help);
	clish_xml_release(completion);
	clish_xml_release(context_help);
	clish_xml_release(inherit);
	clish_xml_release(access);
}

/* ------------------------------------------------------ */
static void
process_config(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_command_t *cmd = (clish_command_t *)parent;
	clish_config_t *config;

	if (!cmd)
		return;
	config = clish_command__get_config(cmd);

	/* read the following text element */
	char *operation = clish_xmlnode_fetch_attr(element, "operation");
	char *priority = clish_xmlnode_fetch_attr(element, "priority");
	char *pattern = clish_xmlnode_fetch_attr(element, "pattern");
	char *file = clish_xmlnode_fetch_attr(element, "file");
	char *splitter = clish_xmlnode_fetch_attr(element, "splitter");
	char *seq = clish_xmlnode_fetch_attr(element, "sequence");
	char *unique = clish_xmlnode_fetch_attr(element, "unique");
	char *depth = clish_xmlnode_fetch_attr(element, "depth");

	if (operation && !lub_string_nocasecmp(operation, "unset"))
		clish_config__set_op(config, CLISH_CONFIG_UNSET);
	else if (operation && !lub_string_nocasecmp(operation, "none"))
		clish_config__set_op(config, CLISH_CONFIG_NONE);
	else if (operation && !lub_string_nocasecmp(operation, "dump"))
		clish_config__set_op(config, CLISH_CONFIG_DUMP);
	else {
		clish_config__set_op(config, CLISH_CONFIG_SET);
		/* The priority if no clearly specified */
		clish_config__set_priority(config, 0x7f00);
	}

	if (priority) {
		long val = 0;
		char *endptr;
		unsigned short pri;

		val = strtol(priority, &endptr, 0);
		if (endptr == priority)
			pri = 0;
		else if (val > 0xffff)
			pri = 0xffff;
		else if (val < 0)
			pri = 0;
		else
			pri = (unsigned short)val;
		clish_config__set_priority(config, pri);
	}

	if (pattern)
		clish_config__set_pattern(config, pattern);
	else
		clish_config__set_pattern(config, "^${__cmd}");

	if (file)
		clish_config__set_file(config, file);

	if (splitter && lub_string_nocasecmp(splitter, "false") == 0)
		clish_config__set_splitter(config, BOOL_FALSE);
	else
		clish_config__set_splitter(config, BOOL_TRUE);

	if (unique && lub_string_nocasecmp(unique, "false") == 0)
		clish_config__set_unique(config, BOOL_FALSE);
	else
		clish_config__set_unique(config, BOOL_TRUE);

	if (seq)
		clish_config__set_seq(config, seq);
	else
		/* The entries without sequence cannot be non-unique */
		clish_config__set_unique(config, BOOL_TRUE);

	if (depth)
		clish_config__set_depth(config, depth);

	clish_xml_release(operation);
	clish_xml_release(priority);
	clish_xml_release(pattern);
	clish_xml_release(file);
	clish_xml_release(splitter);
	clish_xml_release(seq);
	clish_xml_release(unique);
	clish_xml_release(depth);
}

/* ------------------------------------------------------ */
static void process_var(clish_shell_t * shell, clish_xmlnode_t * element, void *parent)
{
	clish_var_t *var = NULL;
	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *dynamic = clish_xmlnode_fetch_attr(element, "dynamic");
	char *value = clish_xmlnode_fetch_attr(element, "value");

	assert(name);

	/* Check if this var doesn't already exist */
	var = (clish_var_t *)lub_bintree_find(&shell->var_tree, name);
	if (var) {
		printf("DUPLICATE VAR: %s\n", name);
		assert(!var);
	}

	/* Create var instance */
	var = clish_var_new(name);
	lub_bintree_insert(&shell->var_tree, var);

	if (dynamic && lub_string_nocasecmp(dynamic, "true") == 0)
		clish_var__set_dynamic(var, BOOL_TRUE);

	if (value)
		clish_var__set_value(var, value);

	clish_xml_release(name);
	clish_xml_release(dynamic);
	clish_xml_release(value);

	process_children(shell, element, var);
}

/* ------------------------------------------------------ */
static void process_wdog(clish_shell_t *shell,
	clish_xmlnode_t *element, void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	clish_command_t *cmd = NULL;

	assert(!shell->wdog);

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "watchdog", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* Remember this command */
	shell->wdog = cmd;

	process_children(shell, element, cmd);
}

/* ------------------------------------------------------ */
static void
process_hotkey(clish_shell_t *shell, clish_xmlnode_t* element, void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;

	char *key = clish_xmlnode_fetch_attr(element, "key");
	char *cmd = clish_xmlnode_fetch_attr(element, "cmd");

	assert(key);
	assert(cmd);

	assert (!clish_view_insert_hotkey(v, key, cmd));

	clish_xml_release(key);
	clish_xml_release(cmd);
}

/* ------------------------------------------------------ */
int clish_shell_xml_read(clish_shell_t * shell, const char *filename)
{
	int ret = -1;
	clish_xmldoc_t *doc;

	doc = clish_xmldoc_read(filename);

	if (clish_xmldoc_is_valid(doc)) {
		clish_xmlnode_t *root = clish_xmldoc_get_root(doc);
		process_node(shell, root, NULL);
		ret = 0;
	} else {
		int errcaps = clish_xmldoc_error_caps(doc);
		printf("Unable to open file '%s'", filename);
		if ((errcaps & CLISH_XMLERR_LINE) == CLISH_XMLERR_LINE)
			printf(", at line %d", clish_xmldoc_get_err_line(doc));
		if ((errcaps & CLISH_XMLERR_COL) == CLISH_XMLERR_COL)
			printf(", at column %d", clish_xmldoc_get_err_col(doc));
		if ((errcaps & CLISH_XMLERR_DESC) == CLISH_XMLERR_DESC)
			printf(", message is %s", clish_xmldoc_get_err_msg(doc));
		printf("\n");
	}

	clish_xmldoc_release(doc);

	return ret;
}

/* ------------------------------------------------------ */
