/*
 * param.c
 *
 * This file provides the implementation of the "param" class
 */
#include "private.h"
#include "lub/string.h"
#include "clish/types.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void clish_param_init(clish_param_t *this, const char *name,
	const char *text, const char *ptype_name)
{
	this->name = lub_string_dup(name);
	this->text = lub_string_dup(text);
	this->ptype_name = lub_string_dup(ptype_name);

	/* Set up defaults */
	this->ptype = NULL;
	this->defval = NULL;
	this->mode = CLISH_PARAM_COMMON;
	this->optional = BOOL_FALSE;
	this->order = BOOL_FALSE;
	this->value = NULL;
	this->hidden = BOOL_FALSE;
	this->test = NULL;
	this->completion = NULL;
	this->access = NULL;

	this->paramv = clish_paramv_new();
}

/*--------------------------------------------------------- */
static void clish_param_fini(clish_param_t * this)
{
	/* deallocate the memory for this instance */
	lub_string_free(this->defval);
	lub_string_free(this->name);
	lub_string_free(this->text);
	lub_string_free(this->ptype_name);
	lub_string_free(this->value);
	lub_string_free(this->test);
	lub_string_free(this->completion);
	lub_string_free(this->access);

	clish_paramv_delete(this->paramv);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
clish_param_t *clish_param_new(const char *name, const char *text,
	const char *ptype_name)
{
	clish_param_t *this = malloc(sizeof(clish_param_t));

	if (this)
		clish_param_init(this, name, text, ptype_name);
	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_param_delete(clish_param_t * this)
{
	clish_param_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_param_insert_param(clish_param_t * this, clish_param_t * param)
{
	return clish_paramv_insert(this->paramv, param);
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
void clish_param__set_ptype_name(clish_param_t *this, const char *ptype_name)
{
	if (this->ptype_name)
		lub_string_free(this->ptype_name);
	this->ptype_name = lub_string_dup(ptype_name);
}

/*--------------------------------------------------------- */
const char * clish_param__get_ptype_name(const clish_param_t *this)
{
	return this->ptype_name;
}

/*--------------------------------------------------------- */
const char *clish_param__get_name(const clish_param_t * this)
{
	if (!this)
		return NULL;
	return this->name;
}

/*--------------------------------------------------------- */
const char *clish_param__get_text(const clish_param_t * this)
{
	return this->text;
}

/*--------------------------------------------------------- */
const char *clish_param__get_range(const clish_param_t * this)
{
	return clish_ptype__get_range(this->ptype);
}

/*--------------------------------------------------------- */
clish_ptype_t *clish_param__get_ptype(const clish_param_t * this)
{
	return this->ptype;
}

/*--------------------------------------------------------- */
void clish_param__set_ptype(clish_param_t *this, clish_ptype_t *ptype)
{
	this->ptype = ptype;
}

/*--------------------------------------------------------- */
void clish_param__set_default(clish_param_t * this, const char *defval)
{
	assert(!this->defval);
	this->defval = lub_string_dup(defval);
}

/*--------------------------------------------------------- */
const char *clish_param__get_default(const clish_param_t * this)
{
	return this->defval;
}

/*--------------------------------------------------------- */
void clish_param__set_mode(clish_param_t * this, clish_param_mode_e mode)
{
	assert(this);
	this->mode = mode;
}

/*--------------------------------------------------------- */
clish_param_mode_e clish_param__get_mode(const clish_param_t * this)
{
	return this->mode;
}

/*--------------------------------------------------------- */
char *clish_param_validate(const clish_param_t * this, const char *text)
{
	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this)) {
		if (lub_string_nocasecmp(clish_param__get_value(this), text))
			return NULL;
	}
	return clish_ptype_translate(this->ptype, text);
}

/*--------------------------------------------------------- */
void clish_param_help(const clish_param_t * this, clish_help_t *help)
{
	const char *range = clish_ptype__get_range(this->ptype);
	const char *name;
	char *str = NULL;

	if (CLISH_PARAM_SWITCH == clish_param__get_mode(this)) {
		unsigned rec_paramc = clish_param__get_param_count(this);
		clish_param_t *cparam;
		unsigned i;

		for (i = 0; i < rec_paramc; i++) {
			cparam = clish_param__get_param(this, i);
			if (!cparam)
				break;
			clish_param_help(cparam, help);
		}
		return;
	}

	if (CLISH_PARAM_SUBCOMMAND == clish_param__get_mode(this))
		name = clish_param__get_value(this);
	else
		if (!(name = clish_ptype__get_text(this->ptype)))
			name = clish_ptype__get_name(this->ptype);

	lub_string_cat(&str, this->text);
	if (range) {
		lub_string_cat(&str, " (");
		lub_string_cat(&str, range);
		lub_string_cat(&str, ")");
	}
	lub_argv_add(help->name, name);
	lub_argv_add(help->help, str);
	lub_string_free(str);
	lub_argv_add(help->detail, NULL);
}

/*--------------------------------------------------------- */
void clish_param_help_arrow(const clish_param_t * this, size_t offset)
{
	fprintf(stderr, "%*c\n", (int)offset, '^');

	this = this; /* Happy compiler */
}

/*--------------------------------------------------------- */
clish_param_t *clish_param__get_param(const clish_param_t * this,
	unsigned index)
{
	return clish_paramv__get_param(this->paramv, index);
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_param__get_paramv(clish_param_t * this)
{
	return this->paramv;
}

/*--------------------------------------------------------- */
unsigned int clish_param__get_param_count(const clish_param_t * this)
{
	return clish_paramv__get_count(this->paramv);
}

/*--------------------------------------------------------- */
void clish_param__set_optional(clish_param_t * this, bool_t optional)
{
	this->optional = optional;
}

/*--------------------------------------------------------- */
bool_t clish_param__get_optional(const clish_param_t * this)
{
	return this->optional;
}

/*--------------------------------------------------------- */
void clish_param__set_order(clish_param_t * this, bool_t order)
{
	this->order = order;
}

/*--------------------------------------------------------- */
bool_t clish_param__get_order(const clish_param_t * this)
{
	return this->order;
}

/*--------------------------------------------------------- */

/* paramv methods */

/*--------------------------------------------------------- */
static void clish_paramv_init(clish_paramv_t * this)
{
	this->paramc = 0;
	this->paramv = NULL;
}

/*--------------------------------------------------------- */
static void clish_paramv_fini(clish_paramv_t * this)
{
	unsigned i;

	/* finalize each of the parameter instances */
	for (i = 0; i < this->paramc; i++) {
		clish_param_delete(this->paramv[i]);
	}
	/* free the parameter vector */
	free(this->paramv);
	this->paramc = 0;
}

/*--------------------------------------------------------- */
clish_paramv_t *clish_paramv_new(void)
{
	clish_paramv_t *this = malloc(sizeof(clish_paramv_t));

	if (this)
		clish_paramv_init(this);
	return this;
}

/*--------------------------------------------------------- */
void clish_paramv_delete(clish_paramv_t * this)
{
	clish_paramv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void clish_paramv_insert(clish_paramv_t * this, clish_param_t * param)
{
	size_t new_size = ((this->paramc + 1) * sizeof(clish_param_t *));
	clish_param_t **tmp;

	/* resize the parameter vector */
	tmp = realloc(this->paramv, new_size);
	if (tmp) {
		this->paramv = tmp;
		/* insert reference to the parameter */
		this->paramv[this->paramc++] = param;
	}
}

/*--------------------------------------------------------- */
int clish_paramv_remove(clish_paramv_t *this, unsigned int index)
{
	size_t new_size;
	clish_param_t **tmp;
	clish_param_t **dst, **src;
	size_t n;

	if (this->paramc < 1)
		return -1;
	if (index >= this->paramc)
		return -1;

	new_size = ((this->paramc - 1) * sizeof(clish_param_t *));
	dst = this->paramv + index;
	src = dst + 1;
	n = this->paramc - index - 1;
	if (n)
		memmove(dst, src, n * sizeof(clish_param_t *));
	/* Resize the parameter vector */
	if (new_size) {
		tmp = realloc(this->paramv, new_size);
		if (!tmp)
			return -1;
		this->paramv = tmp;
	} else {
		free(this->paramv);
		this->paramv = NULL;
	}
	this->paramc--;

	return 0;
}

/*--------------------------------------------------------- */
clish_param_t *clish_paramv__get_param(const clish_paramv_t * this,
	unsigned int index)
{
	clish_param_t *result = NULL;

	if (index < this->paramc)
		result = this->paramv[index];
	return result;
}

/*--------------------------------------------------------- */
clish_param_t *clish_paramv_find_param(const clish_paramv_t * this,
	const char *name)
{
	clish_param_t *res = NULL;
	unsigned int i;

	for (i = 0; i < this->paramc; i++) {
		if (!strcmp(clish_param__get_name(this->paramv[i]), name))
			return this->paramv[i];
		if ((res = clish_paramv_find_param(
			clish_param__get_paramv(this->paramv[i]), name)))
			return res;
	}

	return res;
}

/*--------------------------------------------------------- */
const char *clish_paramv_find_default(const clish_paramv_t * this,
	const char *name)
{
	clish_param_t *res = clish_paramv_find_param(this, name);

	if (res)
		return clish_param__get_default(res);

	return NULL;
}

/*--------------------------------------------------------- */
unsigned int clish_paramv__get_count(const clish_paramv_t * this)
{
	return this->paramc;
}

/*--------------------------------------------------------- */
void clish_param__set_value(clish_param_t * this, const char * value)
{
	assert(!this->value);
	this->value = lub_string_dup(value);
}

/*--------------------------------------------------------- */
char *clish_param__get_value(const clish_param_t * this)
{
	if (this->value)
		return this->value;
	return this->name;
}

/*--------------------------------------------------------- */
void clish_param__set_hidden(clish_param_t * this, bool_t hidden)
{
	this->hidden = hidden;
}

/*--------------------------------------------------------- */
bool_t clish_param__get_hidden(const clish_param_t * this)
{
	return this->hidden;
}

/*--------------------------------------------------------- */
void clish_param__set_test(clish_param_t * this, const char *test)
{
	assert(!this->test);
	this->test = lub_string_dup(test);
}

/*--------------------------------------------------------- */
char *clish_param__get_test(const clish_param_t *this)
{
	return this->test;
}

/*--------------------------------------------------------- */
void clish_param__set_completion(clish_param_t *this, const char *completion)
{
	assert(!this->completion);
	this->completion = lub_string_dup(completion);
}

/*--------------------------------------------------------- */
char *clish_param__get_completion(const clish_param_t *this)
{
	return this->completion;
}

/*--------------------------------------------------------- */
void clish_param__set_access(clish_param_t *this, const char *access)
{
	if (this->access)
		lub_string_free(this->access);
	this->access = lub_string_dup(access);
}

/*--------------------------------------------------------- */
char *clish_param__get_access(const clish_param_t *this)
{
	return this->access;
}
