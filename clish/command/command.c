/*
  * command.c
  *
  * This file provides the implementation of a command definition  
  */
#include "private.h"
#include "clish/types.h"
#include "lub/bintree.h"
#include "lub/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
clish_command_init(clish_command_t *this, const char *name, const char *text)
{
	/* initialise the node part */
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);

	/* set up defaults */
	this->link = NULL;
	this->alias = NULL;
	this->alias_view = NULL;
	this->paramv = clish_paramv_new();
	this->viewid = NULL;
	this->viewname = NULL;
	this->action = clish_action_new();
	this->config = clish_config_new();
	this->detail = NULL;
	this->escape_chars = NULL;
	this->regex_chars = NULL;
	this->args = NULL;
	this->pview = NULL;
	this->lock = BOOL_TRUE;
	this->interrupt = BOOL_FALSE;
	this->dynamic = BOOL_FALSE;
	this->access = NULL;
}

/*--------------------------------------------------------- */
static void clish_command_fini(clish_command_t * this)
{
	lub_string_free(this->name);
	lub_string_free(this->text);

	/* Link need not full cleanup */
	if (this->link)
		return;

	/* finalize each of the parameter instances */
	clish_paramv_delete(this->paramv);

	clish_action_delete(this->action);
	clish_config_delete(this->config);
	lub_string_free(this->alias);
	lub_string_free(this->alias_view);
	lub_string_free(this->viewname);
	lub_string_free(this->viewid);
	lub_string_free(this->detail);
	lub_string_free(this->escape_chars);
	lub_string_free(this->regex_chars);
	lub_string_free(this->access);
	if (this->args)
		clish_param_delete(this->args);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t clish_command_bt_offset(void)
{
	return offsetof(clish_command_t, bt_node);
}

/*--------------------------------------------------------- */
int clish_command_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_command_t *this = clientnode;
	const char *key = clientkey;

	return lub_string_nocasecmp(this->name, key);
}

/*--------------------------------------------------------- */
void clish_command_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_command_t *this = clientnode;

	/* fill out the opaque key */
	strcpy((char *)key, this->name);
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_new(const char *name, const char *help)
{
	clish_command_t *this = malloc(sizeof(clish_command_t));

	if (this)
		clish_command_init(this, name, help);

	return this;
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_new_link(const char *name,
	const char *help, const clish_command_t * ref)
{
	if (!ref)
		return NULL;

	clish_command_t *this = malloc(sizeof(clish_command_t));
	assert(this);

	/* Copy all fields to the new command-link */
	*this = *ref;
	/* Initialise the name (other than original name) */
	this->name = lub_string_dup(name);
	/* Initialise the help (other than original help) */
	this->text = lub_string_dup(help);
	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
	/* It a link to command so set the link flag */
	this->link = ref;

	return this;
}

/*--------------------------------------------------------- */
clish_command_t * clish_command_alias_to_link(clish_command_t *this, clish_command_t *ref)
{
	clish_command_t tmp;

	if (!this || !ref)
		return NULL;
	if (ref->alias) /* The reference is a link too */
		return NULL;
	memcpy(&tmp, this, sizeof(tmp));
	*this = *ref;
	memcpy(&this->bt_node, &tmp.bt_node, sizeof(tmp.bt_node));
	this->name = lub_string_dup(tmp.name);
	this->text = lub_string_dup(tmp.text);
	this->link = ref;
	clish_command_fini(&tmp);

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_command_delete(clish_command_t * this)
{
	clish_command_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_command_insert_param(clish_command_t * this, clish_param_t * param)
{
	clish_paramv_insert(this->paramv, param);
}

/*--------------------------------------------------------- */
int clish_command_help(const clish_command_t *this)
{
	this = this; /* Happy compiler */

	return 0;
}

/*--------------------------------------------------------- */
clish_command_t *clish_command_choose_longest(clish_command_t * cmd1,
	clish_command_t * cmd2)
{
	unsigned len1 = (cmd1 ? strlen(clish_command__get_name(cmd1)) : 0);
	unsigned len2 = (cmd2 ? strlen(clish_command__get_name(cmd2)) : 0);

	if (len2 < len1) {
		return cmd1;
	} else if (len1 < len2) {
		return cmd2;
	} else {
		/* let local view override */
		return cmd1;
	}
}

/*--------------------------------------------------------- */
int clish_command_diff(const clish_command_t * cmd1,
	const clish_command_t * cmd2)
{
	if (NULL == cmd1) {
		if (NULL != cmd2)
			return 1;
		else
			return 0;
	}
	if (NULL == cmd2)
		return -1;

	return lub_string_nocasecmp(clish_command__get_name(cmd1),
		clish_command__get_name(cmd2));
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
const char *clish_command__get_name(const clish_command_t * this)
{
	if (!this)
		return NULL;
	return this->name;
}

/*--------------------------------------------------------- */
const char *clish_command__get_text(const clish_command_t * this)
{
	return this->text;
}

/*--------------------------------------------------------- */
const char *clish_command__get_detail(const clish_command_t * this)
{
	return this->detail;
}

/*--------------------------------------------------------- */
void clish_command__set_detail(clish_command_t * this, const char *detail)
{
	assert(NULL == this->detail);
	this->detail = lub_string_dup(detail);
}

/*--------------------------------------------------------- */
clish_action_t *clish_command__get_action(const clish_command_t *this)
{
	return this->action;
}

/*--------------------------------------------------------- */
clish_config_t *clish_command__get_config(const clish_command_t *this)
{
	return this->config;
}

/*--------------------------------------------------------- */
void clish_command__set_viewname(clish_command_t * this, const char *viewname)
{
	assert(NULL == this->viewname);
	clish_command__force_viewname(this, viewname);
}

/*--------------------------------------------------------- */
void clish_command__force_viewname(clish_command_t * this, const char *viewname)
{
	if (this->viewname)
		lub_string_free(this->viewname);
	this->viewname = lub_string_dup(viewname);
}

/*--------------------------------------------------------- */
char *clish_command__get_viewname(const clish_command_t * this)
{
	return this->viewname;
}

/*--------------------------------------------------------- */
void clish_command__set_viewid(clish_command_t * this, const char *viewid)
{
	assert(NULL == this->viewid);
	clish_command__force_viewid(this, viewid);
}

/*--------------------------------------------------------- */
void clish_command__force_viewid(clish_command_t * this, const char *viewid)
{
	if (this->viewid)
		lub_string_free(this->viewid);
	this->viewid = lub_string_dup(viewid);
}

/*--------------------------------------------------------- */
char *clish_command__get_viewid(const clish_command_t * this)
{
	return this->viewid;
}

/*--------------------------------------------------------- */
const clish_param_t *clish_command__get_param(const clish_command_t * this,
	unsigned index)
{
	return clish_paramv__get_param(this->paramv, index);
}

/*--------------------------------------------------------- */
const char *clish_command__get_suffix(const clish_command_t * this)
{
	return lub_string_suffix(this->name);
}

/*--------------------------------------------------------- */
void
clish_command__set_escape_chars(clish_command_t * this,
	const char *escape_chars)
{
	assert(!this->escape_chars);
	this->escape_chars = lub_string_dup(escape_chars);
}

/*--------------------------------------------------------- */
const char *clish_command__get_escape_chars(const clish_command_t * this)
{
	return this->escape_chars;
}

/*--------------------------------------------------------- */
void clish_command__set_regex_chars(clish_command_t *this,
	const char *escape_chars)
{
	assert(!this->regex_chars);
	this->regex_chars = lub_string_dup(escape_chars);
}

/*--------------------------------------------------------- */
const char *clish_command__get_regex_chars(const clish_command_t *this)
{
	return this->regex_chars;
}

/*--------------------------------------------------------- */
void clish_command__set_args(clish_command_t * this, clish_param_t * args)
{
	assert(NULL == this->args);
	this->args = args;
}

/*--------------------------------------------------------- */
clish_param_t *clish_command__get_args(const clish_command_t * this)
{
	return this->args;
}

/*--------------------------------------------------------- */
unsigned int clish_command__get_param_count(const clish_command_t * this)
{
	return clish_paramv__get_count(this->paramv);
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_command__get_paramv(const clish_command_t * this)
{
	return this->paramv;
}

/*--------------------------------------------------------- */
void clish_command__set_pview(clish_command_t * this, clish_view_t * view)
{
	this->pview = view;
}

/*--------------------------------------------------------- */
clish_view_t *clish_command__get_pview(const clish_command_t * this)
{
	return this->pview;
}

/*--------------------------------------------------------- */
int clish_command__get_depth(const clish_command_t * this)
{
	if (!this->pview)
		return 0;
	return clish_view__get_depth(this->pview);
}

/*--------------------------------------------------------- */
clish_view_restore_e clish_command__get_restore(const clish_command_t * this)
{
	if (!this->pview)
		return CLISH_RESTORE_NONE;
	return clish_view__get_restore(this->pview);
}

/*--------------------------------------------------------- */
const clish_command_t * clish_command__get_orig(const clish_command_t * this)
{
	if (this->link)
		return clish_command__get_orig(this->link);
	return this;
}

/*--------------------------------------------------------- */
bool_t clish_command__get_lock(const clish_command_t * this)
{
	return this->lock;
}

/*--------------------------------------------------------- */
void clish_command__set_lock(clish_command_t * this, bool_t lock)
{
	this->lock = lock;
}

/*--------------------------------------------------------- */
void clish_command__set_alias(clish_command_t * this, const char * alias)
{
	if (this->alias)
		lub_string_free(this->alias);
	this->alias = lub_string_dup(alias);
}

/*--------------------------------------------------------- */
const char * clish_command__get_alias(const clish_command_t * this)
{
	return this->alias;
}

/*--------------------------------------------------------- */
void clish_command__set_alias_view(clish_command_t *this,
	const char *alias_view)
{
	if (this->alias_view)
		lub_string_free(this->alias_view);
	this->alias_view = lub_string_dup(alias_view);
}

/*--------------------------------------------------------- */
const char * clish_command__get_alias_view(const clish_command_t * this)
{
	return this->alias_view;
}

/*--------------------------------------------------------- */
void clish_command__set_dynamic(clish_command_t * this, bool_t dynamic)
{
	this->dynamic = dynamic;
}

/*--------------------------------------------------------- */
bool_t clish_command__get_dynamic(const clish_command_t * this)
{
	return this->dynamic;
}

/*--------------------------------------------------------- */
const clish_command_t * clish_command__get_cmd(const clish_command_t * this)
{
	if (!this->dynamic)
		return this;
	if (this->link)
		return clish_command__get_cmd(this->link);
	return NULL;
}

/*--------------------------------------------------------- */
bool_t clish_command__get_interrupt(const clish_command_t * this)
{
	return this->interrupt;
}

/*--------------------------------------------------------- */
void clish_command__set_interrupt(clish_command_t * this, bool_t interrupt)
{
	this->interrupt = interrupt;
}

/*--------------------------------------------------------- */
void clish_command__set_access(clish_command_t *this, const char *access)
{
	if (this->access)
		lub_string_free(this->access);
	this->access = lub_string_dup(access);
}

/*--------------------------------------------------------- */
char *clish_command__get_access(const clish_command_t *this)
{
	return this->access;
}
