/*
 * shell_var.c
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "lub/string.h"
#include "private.h"

/*----------------------------------------------------------- */
/*
 * search the current viewid string for a variable
 */
void clish_shell__expand_viewid(const char *viewid, lub_bintree_t *tree,
	clish_context_t *context)
{
	char *expanded;
	char *q;
	char *saveptr = NULL;

	expanded = clish_shell_expand(viewid, SHELL_VAR_NONE, context);
	if (!expanded)
		return;

	for (q = strtok_r(expanded, ";", &saveptr);
		q; q = strtok_r(NULL, ";", &saveptr)) {
		char *value;
		clish_var_t *var;

		value = strchr(q, '=');
		if (!value)
			continue;
		*value = '\0';
		value++;
		/* Create var instance */
		var = clish_var_new(q);
		lub_bintree_insert(tree, var);
		clish_var__set_value(var, value);
	}
	lub_string_free(expanded);
}

/*----------------------------------------------------------- */
/*
 * expand context dependent fixed-name variables
 */
static char *find_context_var(const char *name, clish_context_t *this)
{
	char *result = NULL;
	clish_shell_t *shell = this->shell;

	if (!lub_string_nocasecmp(name, "_width")) {
		char tmp[5];
		snprintf(tmp, sizeof(tmp), "%u",
			tinyrl__get_width(shell->tinyrl));
		tmp[sizeof(tmp) - 1] = '\0';
		result = strdup(tmp);

	} else if (!lub_string_nocasecmp(name, "_height")) {
		char tmp[5];
		snprintf(tmp, sizeof(tmp), "%u",
			tinyrl__get_height(shell->tinyrl));
		tmp[sizeof(tmp) - 1] = '\0';
		result = strdup(tmp);

	} else if (!lub_string_nocasecmp(name, "_watchdog_timeout")) {
		char tmp[5];
		snprintf(tmp, sizeof(tmp), "%u", shell->wdog_timeout);
		tmp[sizeof(tmp) - 1] = '\0';
		result = strdup(tmp);

	} else if (!this->cmd) { /* The vars dependent on command */
		return NULL;

	} else if (!lub_string_nocasecmp(name, "_full_cmd")) {
		result = lub_string_dup(clish_command__get_name(this->cmd));

	} else if (!lub_string_nocasecmp(name, "_cmd")) {
		result = lub_string_dup(clish_command__get_name(
			clish_command__get_cmd(this->cmd)));

	} else if (!lub_string_nocasecmp(name, "_orig_cmd")) {
		result = lub_string_dup(clish_command__get_name(
			clish_command__get_orig(this->cmd)));

	} else if (!lub_string_nocasecmp(name, "_line")) {
		result = clish_shell__get_line(this);

	} else if (!lub_string_nocasecmp(name, "_full_line")) {
		result = clish_shell__get_full_line(this);

	} else if (!lub_string_nocasecmp(name, "_params")) {
		if (this->pargv)
			result = clish_shell__get_params(this);

	} else if (!lub_string_nocasecmp(name, "_interactive")) {
		if (clish_shell__get_interactive(this->shell))
			result = strdup("1");
		else
			result = strdup("0");

	} else if (!lub_string_nocasecmp(name, "_isatty")) {
		if (tinyrl__get_isatty(this->shell->tinyrl))
			result = strdup("1");
		else
			result = strdup("0");

	} else if (!lub_string_nocasecmp(name, "_pid")) {
		char tmp[10];
		snprintf(tmp, sizeof(tmp), "%u", getpid());
		tmp[sizeof(tmp) - 1] = '\0';
		result = strdup(tmp);

	} else if (lub_string_nocasestr(name, "_prefix") == name) {
		int idx = 0;
		int pnum = 0;
		pnum = lub_string_wordcount(clish_command__get_name(this->cmd)) -
			lub_string_wordcount(clish_command__get_name(
			clish_command__get_cmd(this->cmd)));
		idx = atoi(name + strlen("_prefix"));
		if (idx < pnum) {
			lub_argv_t *argv = lub_argv_new(
				clish_command__get_name(this->cmd), 0);
			result = lub_string_dup(lub_argv__get_arg(argv, idx));
			lub_argv_delete(argv);
		}

	} else if (!lub_string_nocasecmp(name, "_cur_depth")) {
		char tmp[10];
		int depth = clish_shell__get_depth(shell);
		snprintf(tmp, sizeof(tmp), "%u", depth);
		tmp[sizeof(tmp) - 1] = '\0';
		result = strdup(tmp);

	} else if (!lub_string_nocasecmp(name, "_cur_pwd")) {
		int depth = clish_shell__get_depth(shell);
		result = clish_shell__get_pwd_full(shell, depth);
	}

	return result;
}

/*--------------------------------------------------------- */
static char *find_var(const char *name, lub_bintree_t *tree, clish_context_t *context)
{
	clish_var_t *var = lub_bintree_find(tree, name);
	char *value;
	bool_t dynamic;
	char *res = NULL;

	if (!var)
		return NULL;

	/* Try to get saved value for static var */
	dynamic = clish_var__get_dynamic(var);
	if (!dynamic) {
		char *saved = clish_var__get_saved(var);
		if (saved)
			return lub_string_dup(saved);
	}

	/* Try to expand value field */
	value = clish_var__get_value(var);
	if (value)
		res = clish_shell_expand(value, SHELL_VAR_NONE,  context);

	/* Try to execute ACTION */
	if (!res) {
		char *out = NULL;
		clish_context__set_action(context, clish_var__get_action(var));
		if (clish_shell_exec_action(context, &out, BOOL_FALSE)) {
			lub_string_free(out);
			return NULL;
		}
		res = out;
	}

	/* Save value for static var */
	if (!dynamic && res)
		clish_var__set_saved(var, res);

	return res;
}

/*--------------------------------------------------------- */
static char *find_global_var(const char *name, clish_context_t *context)
{
	clish_shell_t *shell = clish_context__get_shell(context);
	return find_var(name, &shell->var_tree, context);
}

/*--------------------------------------------------------- */
static char *find_viewid_var(const char *name, clish_context_t *context)
{
	clish_shell_t *shell = clish_context__get_shell(context);
	int depth = clish_shell__get_depth(shell);
	if (depth < 0)
		return NULL;
	return find_var(name, &shell->pwdv[depth]->viewid, context);
}

static char * chardiff(const char *syms, const char *minus)
{
	char *dst = malloc(strlen(syms) + 1);
	char *p = dst;
	const char *src;

	for (src = syms; *src; src++) {
		if (!strchr(minus, *src))
			*(p++) = *src;
	}
	*p = '\0';
	return dst;
}

/*--------------------------------------------------------- */
/*
 * return the next segment of text from the provided string
 * segments are delimited by variables within the string.
 */
static char *expand_nextsegment(const char **string, const char *escape_chars,
	clish_context_t *this)
{
	const char *p = *string;
	char *result = NULL;
	size_t len = 0;

	if (!p)
		return NULL;

	if (*p && (p[0] == '$') && (p[1] == '{')) {
		/* start of a variable */
		const char *tmp;
		p += 2;
		tmp = p;

		/*
		 * find the end of the variable 
		 */
		while (*p && p++[0] != '}')
			len++;

		/* ignore non-terminated variables */
		if (p[-1] == '}') {
			bool_t valid = BOOL_FALSE;
			char *text, *q;
			char *saveptr = NULL;

			/* get the variable text */
			text = lub_string_dupn(tmp, len);
			/*
			 * tokenise this INTO ':' separated words
			 * and either expand or duplicate into the result string.
			 * Only return a result if at least 
			 * of the words is an expandable variable
			 */
			for (q = strtok_r(text, ":", &saveptr);
				q; q = strtok_r(NULL, ":", &saveptr)) {
				char *var;
				int mod_quote = 0; /* quote modifier */
				int mod_esc = 0; /* internal escape modifier */
				int mod_esc_chars = 1; /* escaping */
				int mod_esc_dec = 0; /* remove internal chars from escaping */
				char *space;
				char *all_esc = NULL;

				/* Search for modifiers */
				while (*q && !isalpha(*q)) {
					if ('#' == *q) {
						mod_quote = 1;
						mod_esc = 1;
					} else if ('\\' == *q) {
						mod_esc = 1;
					} else if ('!' == *q) {
						mod_quote = 1;
						mod_esc = 1;
						mod_esc_chars = 0;
					} else if ('~' == *q) {
						mod_esc = 1;
						mod_esc_chars = 0;
					/* Internal automatic variable like ${__line} */
					} else if (('_' == *q) && ('_' == *(q+1))) {
						mod_esc_dec = 1;
						q++;
						break;
					/* No escaping at all. Usefull for macros VAR */
					} else if ('^' == *q) {
						mod_quote = 0;
						mod_esc = 0;
						mod_esc_chars = 0;
					} else
						break;
					q++;
				}

				/* Get clean variable value */
				var = clish_shell_expand_var(q, this);
				if (!var) {
					lub_string_cat(&result, q);
					continue;
				}
				valid = BOOL_TRUE;

				/* Quoting */
				if (mod_quote)
					space = strchr(var, ' ');
				if (mod_quote && space)
					lub_string_cat(&result, "\"");

				/* Escape special chars */
				if (escape_chars && mod_esc_chars) {
					/* Remove internal esc from escape chars */
					if (mod_esc_dec)
						all_esc = chardiff(escape_chars,
							lub_string_esc_quoted);
					else
						all_esc = lub_string_dup(escape_chars);
				}

				/* Internal escaping */
				if (mod_esc)
					lub_string_cat(&all_esc,
						lub_string_esc_quoted);

				/* Real escaping */
				if (all_esc) {
					char *tstr = lub_string_encode(var,
						all_esc);
					lub_string_free(var);
					var = tstr;
					lub_string_free(all_esc);
				}

				/* copy the expansion or the raw word */
				lub_string_cat(&result, var);

				/* Quoting */
				if (mod_quote && space)
					lub_string_cat(&result, "\"");

				lub_string_free(var);
			}

			if (!valid) {
				/* not a valid variable expansion */
				lub_string_free(result);
				result = lub_string_dup("");
			}

			/* finished with the variable text */
			lub_string_free(text);
		}
	} else {
		/* find the start of a variable */
		while (*p) {
			if ((p[0] == '$') && (p[1] == '{'))
				break;
			len++;
			p++;
		}
		if (len > 0)
			result = lub_string_dupn(*string, len);
	}
	/* move the string pointer on for next time... */
	*string = p;

	return result;
}

/*--------------------------------------------------------- */
/*
 * This function builds a dynamic string based on that provided
 * subtituting each occurance of a "${FRED}" type variable sub-string
 * with the appropriate value.
 */
char *clish_shell_expand(const char *str, clish_shell_var_e vtype, clish_context_t *context)
{
	char *seg, *result = NULL;
	const char *escape_chars = NULL;
	const clish_command_t *cmd = clish_context__get_cmd(context);

	/* Escape special characters */
	if (SHELL_VAR_REGEX == vtype) {
		if (cmd)
			escape_chars = clish_command__get_regex_chars(cmd);
		if (!escape_chars)
			escape_chars = lub_string_esc_regex;
	} else if (SHELL_VAR_ACTION == vtype) {
		if (cmd)
			escape_chars = clish_command__get_escape_chars(cmd);
		if (!escape_chars)
			escape_chars = lub_string_esc_default;
	}

	/* read each segment and extend the result */
	while ((seg = expand_nextsegment(&str, escape_chars, context))) {
		lub_string_cat(&result, seg);
		lub_string_free(seg);
	}

	return result;
}

/*--------------------------------------------------------- */
char *clish_shell__get_params(clish_context_t *context)
{
	clish_pargv_t *pargv = clish_context__get_pargv(context);
	char *line = NULL;
	unsigned i, cnt;
	const clish_param_t *param;
	const clish_parg_t *parg;
	char *request = NULL;

	if (!pargv)
		return NULL;

	cnt = clish_pargv__get_count(pargv);
	for (i = 0; i < cnt; i++) {
		param = clish_pargv__get_param(pargv, i);
		if (clish_param__get_hidden(param))
			continue;
		parg = clish_pargv__get_parg(pargv, i);
		if (request)
			lub_string_cat(&request, " ");
		lub_string_cat(&request, "${!");
		lub_string_cat(&request, clish_parg__get_name(parg));
		lub_string_cat(&request, "}");
	}

	line = clish_shell_expand(request, SHELL_VAR_NONE, context);
	lub_string_free(request);

	return line;
}

/*--------------------------------------------------------- */
static char *internal_get_line(clish_context_t *context, int cmd_type)
{
	const clish_command_t *cmd = clish_context__get_cmd(context);
	clish_pargv_t *pargv = clish_context__get_pargv(context);
	char *line = NULL;
	char *params = NULL;

	if (0 == cmd_type) /* __cmd */
		lub_string_cat(&line, clish_command__get_name(
			clish_command__get_cmd(cmd)));
	else /* __full_cmd */
		lub_string_cat(&line, clish_command__get_name(cmd));

	if (!pargv)
		return line;

	params = clish_shell__get_params(context);
	if (params) {
		lub_string_cat(&line, " ");
		lub_string_cat(&line, params);
	}
	lub_string_free(params);

	return line;
}

/*--------------------------------------------------------- */
char *clish_shell__get_line(clish_context_t *context)
{
	return internal_get_line(context, 0); /* __cmd */
}

/*--------------------------------------------------------- */
char *clish_shell__get_full_line(clish_context_t *context)
{
	return internal_get_line(context, 1); /* __full_cmd */
}

/*--------------------------------------------------------- */
char *clish_shell_expand_var(const char *name, clish_context_t *context)
{
	return clish_shell_expand_var_ex(name, context, SHELL_EXPAND_ALL);
}

/*----------------------------------------------------------- */
char *clish_shell_expand_var_ex(const char *name, clish_context_t *context, clish_shell_expand_e flags)
{
	clish_shell_t *this;
	const clish_command_t *cmd;
	clish_pargv_t *pargv;
	const char *tmp = NULL;
	char *string = NULL;

	assert(name);
	if (!context)
		return NULL;
	this = clish_context__get_shell(context);
	cmd = clish_context__get_cmd(context);
	pargv = clish_context__get_pargv(context);

	/* try and substitute a parameter value */
	if (pargv && (flags & SHELL_EXPAND_PARAM)) {
		const clish_parg_t *parg = clish_pargv_find_arg(pargv, name);
		if (parg)
			tmp = clish_parg__get_value(parg);
	}

	/* try and substitute the param's default */
	if (!tmp && cmd && (flags & SHELL_EXPAND_PARAM))
		tmp = clish_paramv_find_default(
			clish_command__get_paramv(cmd), name);

	/* try and substitute a viewId variable */
	if (!tmp && this && (flags & SHELL_EXPAND_VIEW))
		tmp = string = find_viewid_var(name, context);

	/* try and substitute context fixed variable */
	if (!tmp && (flags & SHELL_EXPAND_CONTEXT))
		tmp = string = find_context_var(name, context);

	/* try and substitute a global var value */
	if (!tmp && this && (flags & SHELL_EXPAND_VAR))
		tmp = string = find_global_var(name, context);

	/* get the contents of an environment variable */
	if (!tmp && (flags & SHELL_EXPAND_ENV))
		tmp = getenv(name);

	if (string)
		return string;
	return lub_string_dup(tmp);
}

/*----------------------------------------------------------- */
