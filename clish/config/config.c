/*
  * config.c
  *
  * This file provides the implementation of a config definition
  */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "lub/types.h"
#include "lub/string.h"
#include "private.h"


/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void clish_config_init(clish_config_t *this)
{
	this->op = CLISH_CONFIG_NONE;
	this->priority = 0;
	this->pattern = NULL;
	this->file = NULL;
	this->splitter = BOOL_TRUE;
	this->seq = NULL;
	this->unique = BOOL_TRUE;
	this->depth = NULL;
}

/*--------------------------------------------------------- */
static void clish_config_fini(clish_config_t *this)
{
	lub_string_free(this->pattern);
	lub_string_free(this->file);
	lub_string_free(this->seq);
	lub_string_free(this->depth);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
clish_config_t *clish_config_new(void)
{
	clish_config_t *this = malloc(sizeof(clish_config_t));

	if (this)
		clish_config_init(this);

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void clish_config_delete(clish_config_t *this)
{
	clish_config_fini(this);
	free(this);
}

/*---------------------------------------------------------
 * PUBLIC ATTRIBUTES
 *--------------------------------------------------------- */
void clish_config__set_op(clish_config_t *this, clish_config_op_e op)
{
	this->op = op;
}

/*--------------------------------------------------------- */
clish_config_op_e clish_config__get_op(const clish_config_t *this)
{
	return this->op;
}

/*--------------------------------------------------------- */
void clish_config__set_priority(clish_config_t *this, unsigned short priority)
{
	this->priority = priority;
}

/*--------------------------------------------------------- */
unsigned short clish_config__get_priority(const clish_config_t *this)
{
	return this->priority;
}

/*--------------------------------------------------------- */
void clish_config__set_pattern(clish_config_t *this, const char *pattern)
{
	assert(!this->pattern);
	this->pattern = lub_string_dup(pattern);
}

/*--------------------------------------------------------- */
char *clish_config__get_pattern(const clish_config_t *this)
{
	return this->pattern;
}

/*--------------------------------------------------------- */
void clish_config__set_file(clish_config_t *this, const char *file)
{
	assert(!this->file);
	this->file = lub_string_dup(file);
}

/*--------------------------------------------------------- */
char *clish_config__get_file(const clish_config_t *this)
{
	return this->file;
}

/*--------------------------------------------------------- */
bool_t clish_config__get_splitter(const clish_config_t *this)
{
	return this->splitter;
}

/*--------------------------------------------------------- */
void clish_config__set_splitter(clish_config_t *this, bool_t splitter)
{
	this->splitter = splitter;
}

/*--------------------------------------------------------- */
void clish_config__set_seq(clish_config_t *this, const char *seq)
{
	assert(!this->seq);
	this->seq = lub_string_dup(seq);
}

/*--------------------------------------------------------- */
const char *clish_config__get_seq(const clish_config_t *this)
{
	return this->seq;
}

/*--------------------------------------------------------- */
bool_t clish_config__get_unique(const clish_config_t *this)
{
	return this->unique;
}

/*--------------------------------------------------------- */
void clish_config__set_unique(clish_config_t *this, bool_t unique)
{
	this->unique = unique;
}

/*--------------------------------------------------------- */
void clish_config__set_depth(clish_config_t *this, const char *depth)
{
	assert(!this->depth);
	this->depth = lub_string_dup(depth);
}

/*--------------------------------------------------------- */
const char *clish_config__get_depth(const clish_config_t *this)
{
	return this->depth;
}
