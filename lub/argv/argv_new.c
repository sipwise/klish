/*
 * argv_new.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------- */
static void lub_argv_init(lub_argv_t * this, const char *line, size_t offset)
{
	size_t len;
	const char *word;
	lub_arg_t *arg;
	size_t quoted;

	this->argv = NULL;
	this->argc = 0;
	if (!line)
		return;
	/* first of all count the words in the line */
	this->argc = lub_argv_wordcount(line);
	if (0 == this->argc)
		return;
	/* allocate space to hold the vector */
	arg = this->argv = malloc(sizeof(lub_arg_t) * this->argc);
	assert(arg);

	/* then fill out the array with the words */
	for (word = lub_argv_nextword(line, &len, &offset, &quoted);
		*word || quoted;
		word = lub_argv_nextword(word + len, &len, &offset, &quoted)) {
		(*arg).arg = lub_string_ndecode(word, len);
		(*arg).offset = offset;
		(*arg).quoted = quoted ? BOOL_TRUE : BOOL_FALSE;

		offset += len;

		if (quoted) {
			len += quoted - 1; /* account for terminating quotation mark */
			offset += quoted; /* account for quotation marks */
		}
		arg++;
	}
}

/*--------------------------------------------------------- */
lub_argv_t *lub_argv_new(const char *line, size_t offset)
{
	lub_argv_t *this;

	this = malloc(sizeof(lub_argv_t));
	if (this)
		lub_argv_init(this, line, offset);

	return this;
}

/*--------------------------------------------------------- */
void lub_argv_add(lub_argv_t * this, const char *text)
{
	lub_arg_t * arg;

	if (!text)
		return;

	/* allocate space to hold the vector */
	arg = realloc(this->argv, sizeof(lub_arg_t) * (this->argc + 1));
	assert(arg);
	this->argv = arg;
	(this->argv[this->argc++]).arg = strdup(text);
}

/*--------------------------------------------------------- */
static void lub_argv_fini(lub_argv_t * this)
{
	unsigned i;

	for (i = 0; i < this->argc; i++)
		free(this->argv[i].arg);
	free(this->argv);
	this->argv = NULL;
}

/*--------------------------------------------------------- */
void lub_argv_delete(lub_argv_t * this)
{
	lub_argv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
