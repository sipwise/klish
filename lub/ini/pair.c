/*
 * pair.c
 */

#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------- */
int lub_pair_compare(const void *first, const void *second)
{
	const lub_pair_t *f = (const lub_pair_t *)first;
	const lub_pair_t *s = (const lub_pair_t *)second;

	return strcmp(f->name, s->name);
}

/*--------------------------------------------------------- */
void lub_pair_init(lub_pair_t *this, const char *name, const char *value)
{
	assert(this);
	memset(this, 0, sizeof(*this));
	this->name = lub_string_dup(name);
	this->value = lub_string_dup(value);
}

/*--------------------------------------------------------- */
lub_pair_t *lub_pair_new(const char *name, const char *value)
{
	lub_pair_t *this;

	this = malloc(sizeof(*this));
	if (this)
		lub_pair_init(this, name, value);

	return this;
}

/*--------------------------------------------------------- */
void lub_pair_fini(lub_pair_t *this)
{
	assert(this);
	lub_string_free(this->name);
	lub_string_free(this->value);
}

/*--------------------------------------------------------- */
void lub_pair_free(lub_pair_t *this)
{
	assert(this);
	lub_pair_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
const char *lub_pair__get_name(const lub_pair_t *this)
{
	assert(this);
	return this->name;
}

/*--------------------------------------------------------- */
void lub_pair__set_name(lub_pair_t *this, const char *name)
{
	assert(this);
	lub_string_free(this->name);
	this->name = lub_string_dup(name);
}

/*--------------------------------------------------------- */
const char *lub_pair__get_value(const lub_pair_t *this)
{
	assert(this);
	return this->value;
}

/*--------------------------------------------------------- */
void lub_pair__set_value(lub_pair_t *this, const char *value)
{
	assert(this);
	lub_string_free(this->value);
	this->value = lub_string_dup(value);
}

/*--------------------------------------------------------- */
