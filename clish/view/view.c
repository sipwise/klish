/*
 * view.c
 *
 * This file provides the implementation of a view class
 */
#include "private.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int clish_view_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_view_t *this = clientnode;
	const char *key = clientkey;

	return strcmp(this->name, key);
}

/*-------------------------------------------------------- */
void clish_view_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_view_t *this = clientnode;

	/* fill out the opaque key */
	strcpy((char *)key, this->name);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void clish_view_init(clish_view_t * this, const char *name, const char *prompt)
{
	/* set up defaults */
	this->name = lub_string_dup(name);
	this->prompt = NULL;
	this->nspacec = 0;
	this->nspacev = NULL;
	this->depth = 0;
	this->restore = CLISH_RESTORE_NONE;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* initialise the tree of commands for this view */
	lub_bintree_init(&this->tree,
		clish_command_bt_offset(),
		clish_command_bt_compare, clish_command_bt_getkey);

	/* set up the defaults */
	clish_view__set_prompt(this, prompt);

	/* Initialize hotkey structures */
	this->hotkeys = clish_hotkeyv_new();
}

/*--------------------------------------------------------- */
static void clish_view_fini(clish_view_t * this)
{
	clish_command_t *cmd;
	unsigned i;

	/* delete each command held by this view */
	while ((cmd = lub_bintree_findfirst(&this->tree))) {
		/* remove the command from the tree */
		lub_bintree_remove(&this->tree, cmd);
		/* release the instance */
		clish_command_delete(cmd);
	}

	/* free our memory */
	lub_string_free(this->name);
	this->name = NULL;
	lub_string_free(this->prompt);
	this->prompt = NULL;

	/* finalize each of the namespace instances */
	for (i = 0; i < this->nspacec; i++) {
		clish_nspace_delete(this->nspacev[i]);
	}

	/* Free hotkey structures */
	clish_hotkeyv_delete(this->hotkeys);

	/* free the namespace vector */
	free(this->nspacev);
	this->nspacec = 0;
	this->nspacev = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t clish_view_bt_offset(void)
{
	return offsetof(clish_view_t, bt_node);
}

/*--------------------------------------------------------- */
clish_view_t *clish_view_new(const char *name, const char *prompt)
{
	clish_view_t *this = malloc(sizeof(clish_view_t));

	if (this)
		clish_view_init(this, name, prompt);
	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_view_delete(clish_view_t * this)
{
	clish_view_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
clish_command_t *clish_view_new_command(clish_view_t * this,
	const char *name, const char *help)
{
	/* allocate the memory for a new parameter definition */
	clish_command_t *cmd = clish_command_new(name, help);
	assert(cmd);

	/* if this is a command other than the startup command... */
	if (NULL != help) {
		/* ...insert it into the binary tree for this view */
		if (-1 == lub_bintree_insert(&this->tree, cmd)) {
			/* inserting a duplicate command is bad */
			clish_command_delete(cmd);
			cmd = NULL;
		}
	}
	return cmd;
}

/*--------------------------------------------------------- */
/* This method identifies the command (if any) which provides
 * the longest match with the specified line of text.
 *
 * NB this comparison is case insensitive.
 *
 * this - the view instance upon which to operate
 * line - the command line to analyse 
 */
clish_command_t *clish_view_resolve_prefix(clish_view_t * this,
	const char *line, bool_t inherit)
{
	clish_command_t *result = NULL, *cmd;
	char *buffer = NULL;
	lub_argv_t *argv;
	unsigned i;

	/* create a vector of arguments */
	argv = lub_argv_new(line, 0);

	for (i = 0; i < lub_argv__get_count(argv); i++) {
		/* set our buffer to be that of the first "i" arguments  */
		lub_string_cat(&buffer, lub_argv__get_arg(argv, i));

		/* set the result to the longest match */
		cmd = clish_view_find_command(this, buffer, inherit);

		/* job done */
		if (!cmd)
			break;
		result = cmd;

		/* ready for the next word */
		lub_string_cat(&buffer, " ");
	}

	/* free up our dynamic storage */
	lub_string_free(buffer);
	lub_argv_delete(argv);

	return result;
}

/*--------------------------------------------------------- */
clish_command_t *clish_view_resolve_command(clish_view_t *this,
	const char *line, bool_t inherit)
{
	clish_command_t *result = clish_view_resolve_prefix(this, line, inherit);

	if (result) {
		clish_action_t *action = clish_command__get_action(result);
		clish_config_t *config = clish_command__get_config(result);
		if (!clish_action__get_script(action) &&
			(!clish_action__get_builtin(action)) &&
			(CLISH_CONFIG_NONE == clish_config__get_op(config)) &&
			(!clish_command__get_param_count(result)) &&
			(!clish_command__get_view(result))) {
			/* if this doesn't do anything we've
			 * not resolved a command
			 */
			result = NULL;
		}
	}

	return result;
}

/*--------------------------------------------------------- */
clish_command_t *clish_view_find_command(clish_view_t * this,
	const char *name, bool_t inherit)
{
	clish_command_t *cmd, *result = NULL;
	clish_nspace_t *nspace;
	unsigned cnt = clish_view__get_nspace_count(this);
	int i;

	/* Search the current view */
	result = lub_bintree_find(&this->tree, name);
	/* Make command link from command alias */
	result = clish_command_alias_to_link(result);

	if (inherit) {
		for (i = cnt - 1; i >= 0; i--) {
			nspace = clish_view__get_nspace(this, i);
			cmd = clish_nspace_find_command(nspace, name);
			/* choose the longest match */
			result = clish_command_choose_longest(result, cmd);
		}
	}

	return result;
}

/*--------------------------------------------------------- */
static const clish_command_t *find_next_completion(clish_view_t * this,
		const char *iter_cmd, const char *line)
{
	clish_command_t *cmd;
	const char *name = "";
	lub_argv_t *largv;
	unsigned words;

	/* build an argument vector for the line */
	largv = lub_argv_new(line, 0);
	words = lub_argv__get_count(largv);

	/* account for trailing space */
	if (!*line || lub_ctype_isspace(line[strlen(line) - 1]))
		words++;

	if (iter_cmd)
		name = iter_cmd;
	while ((cmd = lub_bintree_findnext(&this->tree, name))) {
		/* Make command link from command alias */
		cmd = clish_command_alias_to_link(cmd);
		name = clish_command__get_name(cmd);
		if (words == lub_argv_wordcount(name)) {
			/* only bother with commands of which this line is a prefix */
			/* this is a completion */
			if (lub_string_nocasestr(name, line) == name)
				break;
		}
	}
	/* clean up the dynamic memory */
	lub_argv_delete(largv);

	return cmd;
}

/*--------------------------------------------------------- */
const clish_command_t *clish_view_find_next_completion(clish_view_t * this,
	const char *iter_cmd, const char *line,
	clish_nspace_visibility_t field, bool_t inherit)
{
	const clish_command_t *result, *cmd;
	clish_nspace_t *nspace;
	unsigned cnt = clish_view__get_nspace_count(this);
	int i;

	/* ask local view for next command */
	result = find_next_completion(this, iter_cmd, line);

	if (!inherit)
		return result;

	/* ask the imported namespaces for next command */
	for (i = cnt - 1; i >= 0; i--) {
		nspace = clish_view__get_nspace(this, i);
		if (!clish_nspace__get_visibility(nspace, field))
			continue;
		cmd = clish_nspace_find_next_completion(nspace,
			iter_cmd, line, field);
		if (clish_command_diff(result, cmd) > 0)
			result = cmd;
	}

	return result;
}

/*--------------------------------------------------------- */
void clish_view_insert_nspace(clish_view_t * this, clish_nspace_t * nspace)
{
	size_t new_size = ((this->nspacec + 1) * sizeof(clish_nspace_t *));
	clish_nspace_t **tmp;

	/* resize the namespace vector */
	tmp = realloc(this->nspacev, new_size);
	assert(tmp);
	this->nspacev = tmp;
	/* insert reference to the namespace */
	this->nspacev[this->nspacec++] = nspace;
}

/*--------------------------------------------------------- */
void clish_view_clean_proxy(clish_view_t * this)
{
	int i;

	/* Iterate namespace instances */
	for (i = 0; i < this->nspacec; i++) {
		clish_nspace_clean_proxy(this->nspacev[i]);
	}
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
const char *clish_view__get_name(const clish_view_t * this)
{
	return this->name;
}

/*--------------------------------------------------------- */
void clish_view__set_prompt(clish_view_t * this, const char *prompt)
{
	assert(!this->prompt);
	this->prompt = lub_string_dup(prompt);
}

/*--------------------------------------------------------- */
char *clish_view__get_prompt(const clish_view_t *this)
{
	return this->prompt;
}

/*--------------------------------------------------------- */
clish_nspace_t *clish_view__get_nspace(const clish_view_t * this,
				       unsigned index)
{
	clish_nspace_t *result = NULL;

	if (index < this->nspacec) {
		result = this->nspacev[index];
	}
	return result;
}

/*--------------------------------------------------------- */
unsigned int clish_view__get_nspace_count(const clish_view_t * this)
{
	return this->nspacec;
}

/*--------------------------------------------------------- */
void clish_view__set_depth(clish_view_t * this, unsigned depth)
{
	this->depth = depth;
}

/*--------------------------------------------------------- */
unsigned clish_view__get_depth(const clish_view_t * this)
{
	return this->depth;
}

/*--------------------------------------------------------- */
void clish_view__set_restore(clish_view_t * this,
	clish_view_restore_t restore)
{
	this->restore = restore;
}

/*--------------------------------------------------------- */
clish_view_restore_t clish_view__get_restore(const clish_view_t * this)
{
	return this->restore;
}

/*--------------------------------------------------------- */
int clish_view_insert_hotkey(const clish_view_t *this, const char *key, const char *cmd)
{
	return clish_hotkeyv_insert(this->hotkeys, key, cmd);
}

/*--------------------------------------------------------- */
const char *clish_view_find_hotkey(const clish_view_t *this, int code)
{
	return clish_hotkeyv_cmd_by_code(this->hotkeys, code);
}

/*--------------------------------------------------------- */
