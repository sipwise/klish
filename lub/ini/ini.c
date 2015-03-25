/*
 * ini.c
 */

#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/*--------------------------------------------------------- */
void lub_ini_init(lub_ini_t *this)
{
	assert(this);
	memset(this, 0, sizeof(*this));
	this->list = lub_list_new(lub_pair_compare);
}

/*--------------------------------------------------------- */
lub_ini_t *lub_ini_new(void)
{
	lub_ini_t *this;

	this = malloc(sizeof(*this));
	if (this)
		lub_ini_init(this);

	return this;
}

/*--------------------------------------------------------- */
void lub_ini_fini(lub_ini_t *this)
{
	lub_list_node_t *iter;

	assert(this);
	while ((iter = lub_list__get_head(this->list))) {
		lub_list_del(this->list, iter);
		lub_pair_free((lub_pair_t *)lub_list_node__get_data(iter));
		lub_list_node_free(iter);
	}
	lub_list_free(this->list);
}

/*--------------------------------------------------------- */
void lub_ini_free(lub_ini_t *this)
{
	assert(this);
	lub_ini_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void lub_ini_add(lub_ini_t *this, lub_pair_t *pair)
{
	assert(this);
	lub_list_add(this->list, pair);
}

/*--------------------------------------------------------- */
/* Find pair by name */
lub_pair_t *lub_ini_find_pair(const lub_ini_t *this, const char *name)
{
	lub_list_node_t *iter;
	lub_pair_t *pair;

	if (!this || !name)
		return NULL;
	/* Iterate elements */
	for(iter = lub_list__get_head(this->list);
		iter; iter = lub_list_node__get_next(iter)) {
		int res;
		pair = (lub_pair_t *)lub_list_node__get_data(iter);
		res = strcmp(lub_pair__get_name(pair), name);
		if (!res)
			return pair;
		if (res > 0) /* No chance to find name */
			break;
	}

	return NULL;
}

/*--------------------------------------------------------- */
/* Find pair by name */
const char *lub_ini_find(const lub_ini_t *this, const char *name)
{
	lub_pair_t *pair = lub_ini_find_pair(this, name);

	if (!pair)
		return NULL;
	return lub_pair__get_value(pair);
}

/*--------------------------------------------------------- */
int lub_ini_parse_str(lub_ini_t *this, const char *ini)
{
	char *buffer;
	char *saveptr = NULL;
	char *line;

	buffer = lub_string_dup(ini);
	/* Now loop though each line */
	for (line = strtok_r(buffer, "\n", &saveptr);
		line; line = strtok_r(NULL, "\n", &saveptr)) {

		char *str, *name, *value, *savestr, *ns = line;
		const char *begin;
		size_t len, offset, quoted;
		char *rname, *rvalue;
		lub_pair_t *pair;

		if (!*ns) /* Empty */
			continue;
		while (*ns && isspace(*ns))
			ns++;
		if ('#' == *ns) /* Comment */
			continue;
		if ('=' == *ns) /* Broken string */
			continue;
		str = lub_string_dup(ns);
		name = strtok_r(str, "=", &savestr);
		if (!name) {
			lub_string_free(str);
			continue;
		}
		value = strtok_r(NULL, "=", &savestr);
		begin = lub_string_nextword(name, &len, &offset, &quoted);
		rname = lub_string_dupn(begin, len);
		if (!value) /* Empty value */
			rvalue = NULL;
		else {
			begin = lub_string_nextword(value, &len, &offset, &quoted);
			rvalue = lub_string_dupn(begin, len);
		}
		pair = lub_pair_new(rname, rvalue);
		lub_ini_add(this, pair);
		lub_string_free(rname);
		lub_string_free(rvalue);
		lub_string_free(str);
	}
	lub_string_free(buffer);

	return 0;
}

/*--------------------------------------------------------- */
int lub_ini_parse_file(lub_ini_t *this, const char *fn)
{
	int ret = -1;
	FILE *f;
	char *buf;
	unsigned int p = 0;
	const int chunk_size = 128;
	int size = chunk_size;

	if (!fn || !*fn)
		return -1;
	f = fopen(fn, "r");
	if (!f)
		return -1;

	buf = malloc(size);
	while (fgets(buf + p, size - p, f)) {
		char *tmp;
		if (feof(f) || strchr(buf + p, '\n') || strchr(buf + p, '\r')) {
			lub_ini_parse_str(this, buf);
			p = 0;
			continue;
		}
		p = size - 1;
		size += chunk_size;
		tmp = realloc(buf, size);
		if (!tmp)
			goto error;
		buf = tmp;
	}

	ret = 0;
error:
	free(buf);
	fclose(f);

	return ret;
}

/*--------------------------------------------------------- */
lub_ini_node_t *lub_ini__get_head(lub_ini_t *this)
{
	return lub_list__get_head(this->list);
}

/*--------------------------------------------------------- */
lub_ini_node_t *lub_ini__get_tail(lub_ini_t *this)
{
	return lub_list__get_tail(this->list);
}

/*--------------------------------------------------------- */

lub_ini_node_t *lub_ini__get_next(lub_ini_node_t *node)
{
	return lub_list_node__get_next(node);
}

/*--------------------------------------------------------- */

lub_ini_node_t *lub_ini__get_prev(lub_ini_node_t *node)
{
	return lub_list_node__get_next(node);
}

/*--------------------------------------------------------- */

lub_pair_t *lub_ini__iter_data(lub_ini_node_t *node)
{
	return (lub_pair_t *)lub_list_node__get_data(node);
}

/*--------------------------------------------------------- */
