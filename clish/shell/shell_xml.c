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

typedef int (PROCESS_FN) (clish_shell_t *instance,
	clish_xmlnode_t *element, void *parent);

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
	process_hotkey,
	process_plugin,
	process_hook;

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
	{"PLUGIN", process_plugin},
	{"HOOK", process_hook},
	{NULL, NULL}
};

/*
 * if CLISH_PATH is unset in the environment then this is the value used. 
 */
const char *default_path = "/etc/clish;~/.clish";

static int process_node(clish_shell_t *shell, clish_xmlnode_t *node,
	void *parent);

/*-------------------------------------------------------- */
int clish_shell_load_scheme(clish_shell_t *this, const char *xml_path, const char *xslt_path)
{
	const char *path = xml_path;
	char *buffer;
	char *dirname;
	char *saveptr = NULL;
	int res = -1;
	clish_xmldoc_t *doc = NULL;
	DIR *dir;

#ifdef HAVE_LIB_LIBXSLT
	clish_xslt_t *xslt = NULL;

	/* Load global XSLT stylesheet */
	if (xslt_path) {
		xslt = clish_xslt_read(xslt_path);
		if (!clish_xslt_is_valid(xslt)) {
			fprintf(stderr, CLISH_XML_ERROR_STR"Can't load XSLT file %s\n",
				xslt_path);
			return -1;
		}
	}
#endif

	/* Use the default path */
	if (!path)
		path = default_path;
	buffer = lub_system_tilde_expand(path);

	/* Loop though each directory */
	for (dirname = strtok_r(buffer, ";", &saveptr);
		dirname; dirname = strtok_r(NULL, ";", &saveptr)) {
		struct dirent *entry;

		/* Search this directory for any XML files */
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
			char *filename = NULL;
			clish_xmlnode_t *root;
			int r;

			/* Check the filename */
			if (!extension || strcmp(".xml", extension))
				continue;

			/* Build the filename */
			lub_string_cat(&filename, dirname);
			lub_string_cat(&filename, "/");
			lub_string_cat(&filename, entry->d_name);

#ifdef DEBUG
			fprintf(stderr, "Parse XML-file: %s\n", filename);
#endif
			/* Load current XML file */
			doc = clish_xmldoc_read(filename);
			if (!clish_xmldoc_is_valid(doc)) {
				int errcaps = clish_xmldoc_error_caps(doc);
				printf("Unable to open file '%s'", filename);
				if ((errcaps & CLISH_XMLERR_LINE) == CLISH_XMLERR_LINE)
					printf(", at line %d", clish_xmldoc_get_err_line(doc));
				if ((errcaps & CLISH_XMLERR_COL) == CLISH_XMLERR_COL)
					printf(", at column %d", clish_xmldoc_get_err_col(doc));
				if ((errcaps & CLISH_XMLERR_DESC) == CLISH_XMLERR_DESC)
					printf(", message is %s", clish_xmldoc_get_err_msg(doc));
				printf("\n");
				goto error;
			}
#ifdef HAVE_LIB_LIBXSLT
			/* Use embedded stylesheet if stylesheet
			 * filename is not specified.
			 */
			if (!xslt_path)
				xslt = clish_xslt_read_embedded(doc);

			if (clish_xslt_is_valid(xslt)) {
				clish_xmldoc_t *tmp = NULL;
				tmp = clish_xslt_apply(doc, xslt);
				if (!clish_xmldoc_is_valid(tmp)) {
					fprintf(stderr, CLISH_XML_ERROR_STR"Can't load XSLT file %s\n", xslt_path);
					goto error;
				}
				clish_xmldoc_release(doc);
				doc = tmp;
			}

			if (!xslt_path && clish_xslt_is_valid(xslt))
				clish_xslt_release(xslt);
#endif
			root = clish_xmldoc_get_root(doc);
			r = process_node(this, root, NULL);
			clish_xmldoc_release(doc);

			/* Error message */
			if (r) {
				fprintf(stderr, CLISH_XML_ERROR_STR"File %s\n",
					filename);
				lub_string_free(filename);
				goto error;
			}
			lub_string_free(filename);
		}
		closedir(dir);
	}

/* To don't free memory twice on cleanup */
#ifdef HAVE_LIB_LIBXSLT
	if (!xslt_path)
		xslt = NULL;
#endif
	doc = NULL;
	dir = NULL;
	res = 0; /* Success */
error:
	lub_string_free(buffer);
	if (dir)
		closedir(dir);
	if (clish_xmldoc_is_valid(doc))
		clish_xmldoc_release(doc);
#ifdef HAVE_LIB_LIBXSLT
	if (clish_xslt_is_valid(xslt))
		clish_xslt_release(xslt);
#endif

	return res;
}

/*
 * ------------------------------------------------------
 * This function reads an element from the XML stream and processes it.
 * ------------------------------------------------------
 */
static int process_node(clish_shell_t *shell, clish_xmlnode_t *node,
	void *parent)
{
	int res = 0;

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
						res = cb->handler(shell, node, parent);

						/* Error message */
						if (res) {
							char *ename = clish_xmlnode_fetch_attr(node, "name");
							char *eref = clish_xmlnode_fetch_attr(node, "ref");
							char *ekey = clish_xmlnode_fetch_attr(node, "key");
							char *efile = clish_xmlnode_fetch_attr(node, "file");
							fprintf(stderr, CLISH_XML_ERROR_STR"Node %s", name);
							if (ename)
								fprintf(stderr, ", name=\"%s\"", ename);
							if (eref)
								fprintf(stderr, ", ref=\"%s\"", eref);
							if (ekey)
								fprintf(stderr, ", key=\"%s\"", ekey);
							if (efile)
								fprintf(stderr, ", file=\"%s\"", efile);
							fprintf(stderr, "\n");
							clish_xml_release(ename);
							clish_xml_release(eref);
							clish_xml_release(ekey);
							clish_xml_release(efile);
						}
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

	return res;
}

/* ------------------------------------------------------ */
static int process_children(clish_shell_t *shell,
	clish_xmlnode_t *element, void *parent)
{
	clish_xmlnode_t *node = NULL;
	int res;

	while ((node = clish_xmlnode_next_child(element, node)) != NULL) {
		/* Now deal with all the contained elements */
		res = process_node(shell, node, parent);
		if (res)
			return res;
	}

	return 0;
}

/* ------------------------------------------------------ */
static int process_clish_module(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	/* Create the global view */
	if (!shell->global)
		shell->global = clish_shell_find_create_view(shell,
			"__view_global", "");

	parent = parent; /* Happy compiler */

	return process_children(shell, element, shell->global);
}

/* ------------------------------------------------------ */
static int process_view(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_view_t *view;
	int res = -1;

	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *prompt = clish_xmlnode_fetch_attr(element, "prompt");
	char *depth = clish_xmlnode_fetch_attr(element, "depth");
	char *restore = clish_xmlnode_fetch_attr(element, "restore");
	char *access = clish_xmlnode_fetch_attr(element, "access");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}

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

	if (access)
		clish_view__set_access(view, access);

//process_view_end:
	res = process_children(shell, element, view);
error:
	clish_xml_release(name);
	clish_xml_release(prompt);
	clish_xml_release(depth);
	clish_xml_release(restore);
	clish_xml_release(access);

	parent = parent; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
static int process_ptype(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	int res = -1;

	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *help = clish_xmlnode_fetch_attr(element, "help");
	char *pattern = clish_xmlnode_fetch_attr(element, "pattern");
	char *method_name = clish_xmlnode_fetch_attr(element, "method");
	char *preprocess_name =	clish_xmlnode_fetch_attr(element, "preprocess");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}
	if (!pattern) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("pattern"));
		goto error;
	}

	method = clish_ptype_method_resolve(method_name);

	preprocess = clish_ptype_preprocess_resolve(preprocess_name);
	clish_shell_find_create_ptype(shell,
		name, help, pattern, method, preprocess);

	res = 0;
error:
	clish_xml_release(name);
	clish_xml_release(help);
	clish_xml_release(pattern);
	clish_xml_release(method_name);
	clish_xml_release(preprocess_name);

	parent = parent; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
static int process_overview(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
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
		char *new = (char*)realloc(content, content_len);
		if (!new) {
			if (content)
				free(content);
			return -1;
		}
		content = new;
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

	parent = parent; /* Happy compiler */

	return 0;
}

/* ------------------------------------------------------ */
static int process_command(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	clish_command_t *old;
	int res = -1;

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

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}
	if (!help) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("help"));
		goto error;
	}

	/* check this command doesn't already exist */
	old = clish_view_find_command(v, name, BOOL_FALSE);
	if (old) {
		fprintf(stderr, CLISH_XML_ERROR_STR"Duplicate COMMAND name=\"%s\".\n", name);
		goto error;
	}

	/* create a command */
	cmd = clish_view_new_command(v, name, help);
	clish_command__set_pview(cmd, v);

	/* Reference 'ref' field */
	if (ref) {
		char *saveptr = NULL;
		const char *delim = "@";
		char *view_name = NULL;
		char *cmdn = NULL;
		char *str = lub_string_dup(ref);

		cmdn = strtok_r(str, delim, &saveptr);
		if (!cmdn) {
			fprintf(stderr, CLISH_XML_ERROR_STR"Invalid \"ref\" attribute value.\n");
			lub_string_free(str);
			goto error;
		}
		clish_command__set_alias(cmd, cmdn); /* alias name */
		view_name = strtok_r(NULL, delim, &saveptr);
		clish_command__set_alias_view(cmd, view_name);
		lub_string_free(str);
	}

	/* define some specialist escape characters */
	if (escape_chars)
		clish_command__set_escape_chars(cmd, escape_chars);

	if (args_name) {
		/* define a "rest of line" argument */
		clish_param_t *param;

		/* Check syntax */
		if (!args_help) {
			fprintf(stderr, CLISH_XML_ERROR_ATTR("args_help"));
			goto error;
		}
		param = clish_param_new(args_name, args_help, "__ptype_ARGS");
		clish_command__set_args(cmd, param);
	}

	/* define the view which this command changes to */
	if (view) {
		/* reference the next view */
		clish_command__set_viewname(cmd, view);
	}

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

	if (access)
		clish_command__set_access(cmd, access);

//process_command_end:
	res = process_children(shell, element, cmd);
error:
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

	return res;
}

/* ------------------------------------------------------ */
static int process_startup(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *) parent;
	clish_command_t *cmd = NULL;
	int res = -1;

	char *view = clish_xmlnode_fetch_attr(element, "view");
	char *viewid = clish_xmlnode_fetch_attr(element, "viewid");
	char *default_shebang =
		clish_xmlnode_fetch_attr(element, "default_shebang");
	char *timeout = clish_xmlnode_fetch_attr(element, "timeout");
	char *lock = clish_xmlnode_fetch_attr(element, "lock");
	char *interrupt = clish_xmlnode_fetch_attr(element, "interrupt");
	char *default_plugin = clish_xmlnode_fetch_attr(element,
		"default_plugin");

	/* Check syntax */
	if (!view) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("view"));
		goto error;
	}
	if (shell->startup) {
		fprintf(stderr, CLISH_XML_ERROR_STR"STARTUP tag duplication.\n");
		goto error;
	}

	/* create a command with NULL help */
	cmd = clish_view_new_command(v, "startup", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* reference the next view */
	clish_command__set_viewname(cmd, view);

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

	/* If we need the default plugin */
	if (default_plugin && (0 == strcmp(default_plugin, "false")))
		shell->default_plugin = BOOL_FALSE;

	/* remember this command */
	shell->startup = cmd;

	res = process_children(shell, element, cmd);
error:
	clish_xml_release(view);
	clish_xml_release(viewid);
	clish_xml_release(default_shebang);
	clish_xml_release(timeout);
	clish_xml_release(lock);
	clish_xml_release(interrupt);

	return res;
}

/* ------------------------------------------------------ */
static int process_param(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_command_t *cmd = NULL;
	clish_param_t *p_param = NULL;
	clish_xmlnode_t *pelement;
	clish_param_t *param;
	char *pname;
	int res = -1;

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
	char *access = clish_xmlnode_fetch_attr(element, "access");

	/* The PARAM can be child of COMMAND or another PARAM */
	pelement = clish_xmlnode_parent(element);
	pname = clish_xmlnode_get_all_name(pelement);
	if (pname && lub_string_nocasecmp(pname, "PARAM") == 0)
		p_param = (clish_param_t *)parent;
	else
		cmd = (clish_command_t *)parent;
	if (pname)
		free(pname);
	if (!cmd && !p_param)
		goto error;

	/* Check syntax */
	if (cmd && (cmd == shell->startup)) {
		fprintf(stderr, CLISH_XML_ERROR_STR"STARTUP can't contain PARAMs.\n");
		goto error;
	}
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}
	if (!help) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("help"));
		goto error;
	}
	if (!ptype) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("ptype"));
		goto error;
	}

	param = clish_param_new(name, help, ptype);

	/* If prefix is set clish will emulate old optional
	 * command syntax over newer optional command mechanism.
	 * It will create nested PARAM.
	 */
	if (prefix) {
		const char *ptype_name = "__ptype_SUBCOMMAND";
		clish_param_t *opt_param = NULL;
		char *str = NULL;
		clish_ptype_t *tmp;

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
		lub_string_cat(&str, "_prefix_");
		lub_string_cat(&str, name);
		opt_param = clish_param_new(str, help, ptype_name);
		lub_string_free(str);
		clish_param__set_mode(opt_param,
			CLISH_PARAM_SUBCOMMAND);
		clish_param__set_value(opt_param, prefix);
		clish_param__set_optional(opt_param, BOOL_TRUE);

		if (test)
			clish_param__set_test(opt_param, test);

		/* Add the parameter to the command */
		if (cmd)
			clish_command_insert_param(cmd, opt_param);
		/* Add the parameter to the param */
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

	if (access)
		clish_param__set_access(param, access);

	/* Add the parameter to the command */
	if (cmd)
		clish_command_insert_param(cmd, param);

	/* Add the parameter to the param */
	if (p_param)
		clish_param_insert_param(p_param, param);

	res = process_children(shell, element, param);

error:
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
	clish_xml_release(access);

	return res;
}

/* ------------------------------------------------------ */
static int process_action(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_action_t *action = NULL;
	char *builtin = clish_xmlnode_fetch_attr(element, "builtin");
	char *shebang = clish_xmlnode_fetch_attr(element, "shebang");
	clish_xmlnode_t *pelement = clish_xmlnode_parent(element);
	char *pname = clish_xmlnode_get_all_name(pelement);
	char *text;
	clish_sym_t *sym = NULL;

	if (pname && lub_string_nocasecmp(pname, "VAR") == 0)
		action = clish_var__get_action((clish_var_t *)parent);
	else
		action = clish_command__get_action((clish_command_t *)parent);

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
		sym = clish_shell_add_unresolved_sym(shell, builtin,
			CLISH_SYM_TYPE_ACTION);
	else
		sym = shell->hooks[CLISH_SYM_TYPE_ACTION];

	clish_action__set_builtin(action, sym);
	if (shebang)
		clish_action__set_shebang(action, shebang);

	clish_xml_release(builtin);
	clish_xml_release(shebang);

	return 0;
}

/* ------------------------------------------------------ */
static int process_detail(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
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

	shell = shell; /* Happy compiler */

	return 0;
}

/* ------------------------------------------------------ */
static int process_namespace(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	clish_nspace_t *nspace = NULL;
	int res = -1;

	char *view = clish_xmlnode_fetch_attr(element, "ref");
	char *prefix = clish_xmlnode_fetch_attr(element, "prefix");
	char *prefix_help = clish_xmlnode_fetch_attr(element, "prefix_help");
	char *help = clish_xmlnode_fetch_attr(element, "help");
	char *completion = clish_xmlnode_fetch_attr(element, "completion");
	char *context_help = clish_xmlnode_fetch_attr(element, "context_help");
	char *inherit = clish_xmlnode_fetch_attr(element, "inherit");
	char *access = clish_xmlnode_fetch_attr(element, "access");

	/* Check syntax */
	if (!view) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("ref"));
		goto error;
	}

	clish_view_t *ref_view = clish_shell_find_view(shell, view);

	/* Don't include itself without prefix */
	if ((ref_view == v) && !prefix)
		goto process_namespace_end;

	nspace = clish_nspace_new(view);
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

	if (access)
		clish_nspace__set_access(nspace, access);

process_namespace_end:
	res = 0;
error:
	clish_xml_release(view);
	clish_xml_release(prefix);
	clish_xml_release(prefix_help);
	clish_xml_release(help);
	clish_xml_release(completion);
	clish_xml_release(context_help);
	clish_xml_release(inherit);
	clish_xml_release(access);

	return res;
}

/* ------------------------------------------------------ */
static int process_config(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_command_t *cmd = (clish_command_t *)parent;
	clish_config_t *config;

	if (!cmd)
		return 0;
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

	shell = shell; /* Happy compiler */

	return 0;
}

/* ------------------------------------------------------ */
static int process_var(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_var_t *var = NULL;
	int res = -1;

	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *dynamic = clish_xmlnode_fetch_attr(element, "dynamic");
	char *value = clish_xmlnode_fetch_attr(element, "value");

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}
	/* Check if this var doesn't already exist */
	var = (clish_var_t *)lub_bintree_find(&shell->var_tree, name);
	if (var) {
		fprintf(stderr, CLISH_XML_ERROR_STR"Duplicate VAR name=\"%s\".\n", name);
		goto error;
	}

	/* Create var instance */
	var = clish_var_new(name);
	lub_bintree_insert(&shell->var_tree, var);

	if (dynamic && lub_string_nocasecmp(dynamic, "true") == 0)
		clish_var__set_dynamic(var, BOOL_TRUE);

	if (value)
		clish_var__set_value(var, value);

	res = process_children(shell, element, var);
error:
	clish_xml_release(name);
	clish_xml_release(dynamic);
	clish_xml_release(value);

	parent = parent; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
static int process_wdog(clish_shell_t *shell,
	clish_xmlnode_t *element, void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	clish_command_t *cmd = NULL;
	int res = -1;

	/* Check syntax */
	if (shell->wdog) {
		fprintf(stderr, CLISH_XML_ERROR_STR"WATCHDOG tag duplication.\n");
		goto error;
	}

	/* Create a command with NULL help */
	cmd = clish_view_new_command(v, "watchdog", NULL);
	clish_command__set_lock(cmd, BOOL_FALSE);

	/* Remember this command */
	shell->wdog = cmd;

	res = process_children(shell, element, cmd);
error:
	return res;
}

/* ------------------------------------------------------ */
static int process_hotkey(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_view_t *v = (clish_view_t *)parent;
	int res = -1;

	char *key = clish_xmlnode_fetch_attr(element, "key");
	char *cmd = clish_xmlnode_fetch_attr(element, "cmd");

	/* Check syntax */
	if (!key) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("key"));
		goto error;
	}
	if (!cmd) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("cmd"));
		goto error;
	}

	clish_view_insert_hotkey(v, key, cmd);

	res = 0;
error:
	clish_xml_release(key);
	clish_xml_release(cmd);

	shell = shell; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
static int process_plugin(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	clish_plugin_t *plugin;
	char *file = clish_xmlnode_fetch_attr(element, "file");
	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *alias = clish_xmlnode_fetch_attr(element, "alias");
	char *rtld_global = clish_xmlnode_fetch_attr(element, "rtld_global");
	int res = -1;
	char *text;

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}

	plugin = clish_shell_find_plugin(shell, name);
	if (plugin) {
		fprintf(stderr,
			CLISH_XML_ERROR_STR"PLUGIN %s duplication.\n", name);
		goto error;
	}
	plugin = clish_plugin_new(name);
	lub_list_add(shell->plugins, plugin);

	if (alias && *alias)
		clish_plugin__set_alias(plugin, alias);

	if (file && *file)
		clish_plugin__set_file(plugin, file);

	if (rtld_global && lub_string_nocasecmp(rtld_global, "true") == 0)
		clish_plugin__set_rtld_global(plugin, BOOL_TRUE);

	/* Get PLUGIN body content */
	text = clish_xmlnode_get_all_content(element);
	if (text && *text)
		clish_plugin__set_conf(plugin, text);
	if (text)
		free(text);

	res = 0;
error:
	clish_xml_release(file);
	clish_xml_release(name);
	clish_xml_release(alias);
	clish_xml_release(rtld_global);

	parent = parent; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
static int process_hook(clish_shell_t *shell, clish_xmlnode_t *element,
	void *parent)
{
	char *name = clish_xmlnode_fetch_attr(element, "name");
	char *builtin = clish_xmlnode_fetch_attr(element, "builtin");
	int res = -1;
	int type = CLISH_SYM_TYPE_NONE;

	/* Check syntax */
	if (!name) {
		fprintf(stderr, CLISH_XML_ERROR_ATTR("name"));
		goto error;
	}
	/* Find out HOOK type */
	if (!strcmp(name, "action"))
		type = CLISH_SYM_TYPE_ACTION;
	else if (!strcmp(name, "access"))
		type = CLISH_SYM_TYPE_ACCESS;
	else if (!strcmp(name, "config"))
		type = CLISH_SYM_TYPE_CONFIG;
	else if (!strcmp(name, "log"))
		type = CLISH_SYM_TYPE_LOG;
	if (CLISH_SYM_TYPE_NONE == type) {
		fprintf(stderr, CLISH_XML_ERROR_STR"Unknown HOOK name %s.\n", name);
		goto error;
	}

	/* Check duplicate HOOK tag */
	if (shell->hooks_use[type]) {
		fprintf(stderr,
			CLISH_XML_ERROR_STR"HOOK %s duplication.\n", name);
		goto error;
	}
	shell->hooks_use[type] = BOOL_TRUE;
	clish_sym__set_name(shell->hooks[type], builtin);

	res = 0;
error:
	clish_xml_release(name);
	clish_xml_release(builtin);

	parent = parent; /* Happy compiler */

	return res;
}

/* ------------------------------------------------------ */
