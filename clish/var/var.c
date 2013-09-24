/*
 * var.c
 *
 * This file provides the implementation of the "var" class
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lub/string.h"
#include "private.h"

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void clish_var_init(clish_var_t *this, const char *name)
{
	this->name = lub_string_dup(name);
	this->dynamic = BOOL_FALSE;
	this->value = NULL;
	this->action = clish_action_new();
	this->saved = NULL;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
}

/*--------------------------------------------------------- */
static void clish_var_fini(clish_var_t *this)
{
	lub_string_free(this->name);
	lub_string_free(this->value);
	clish_action_delete(this->action);
	lub_string_free(this->saved);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
int clish_var_bt_compare(const void *clientnode, const void *clientkey)
{
	const clish_var_t *this = clientnode;
	const char *key = clientkey;

	return strcmp(this->name, key);
}

/*-------------------------------------------------------- */
void clish_var_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const clish_var_t *this = clientnode;

	/* fill out the opaque key */
	strcpy((char *)key, this->name);
}

/*--------------------------------------------------------- */
size_t clish_var_bt_offset(void)
{
	return offsetof(clish_var_t, bt_node);
}

/*--------------------------------------------------------- */
clish_var_t *clish_var_new(const char *name)
{
	clish_var_t *this = malloc(sizeof(clish_var_t));
	if (this)
		clish_var_init(this, name);
	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_var_delete(clish_var_t *this)
{
	clish_var_fini(this);
	free(this);
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
const char *clish_var__get_name(const clish_var_t *this)
{
	if (!this)
		return NULL;
	return this->name;
}

/*--------------------------------------------------------- */
void clish_var__set_dynamic(clish_var_t *this, bool_t dynamic)
{
	this->dynamic = dynamic;
}

/*--------------------------------------------------------- */
bool_t clish_var__get_dynamic(const clish_var_t *this)
{
	return this->dynamic;
}

/*--------------------------------------------------------- */
void clish_var__set_value(clish_var_t *this, const char *value)
{
	if (this->value)
		lub_string_free(this->value);
	this->value = lub_string_dup(value);
}

/*--------------------------------------------------------- */
char *clish_var__get_value(const clish_var_t *this)
{
	return this->value;
}

/*--------------------------------------------------------- */
clish_action_t *clish_var__get_action(const clish_var_t *this)
{
	return this->action;
}

/*--------------------------------------------------------- */
void clish_var__set_saved(clish_var_t *this, const char *value)
{
	if (this->saved)
		lub_string_free(this->saved);
	this->saved = lub_string_dup(value);
}

/*--------------------------------------------------------- */
char *clish_var__get_saved(const clish_var_t *this)
{
	return this->saved;
}
