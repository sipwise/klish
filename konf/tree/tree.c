/*
 * tree.c
 *
 * This file provides the implementation of a konf_tree class
 */

#include "private.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
static int konf_tree_compare(const void *first, const void *second)
{
	const konf_tree_t *f = (const konf_tree_t *)first;
	const konf_tree_t *s = (const konf_tree_t *)second;

	/* Priority check */
	if (f->priority != s->priority)
		return (f->priority - s->priority);
	/* Sequence check */
	if (f->seq_num != s->seq_num)
		return (f->seq_num - s->seq_num);
	/* Sub-sequence check */
	if (f->sub_num != s->sub_num)
		return (f->sub_num - s->sub_num);
	/* Line check */
	return strcmp(f->line, s->line);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void konf_tree_init(konf_tree_t * this, const char *line,
	unsigned short priority)
{
	/* set up defaults */
	this->line = strdup(line);
	this->priority = priority;
	this->seq_num = 0;
	this->sub_num = KONF_ENTRY_OK;
	this->splitter = BOOL_TRUE;
	this->depth = -1;

	/* initialise the list of commands for this conf */
	this->list = lub_list_new(konf_tree_compare);
}

/*--------------------------------------------------------- */
static void konf_tree_fini(konf_tree_t * this)
{
	lub_list_node_t *iter;

	/* delete each conf held by this conf */
	
	while ((iter = lub_list__get_head(this->list))) {
		/* remove the conf from the tree */
		lub_list_del(this->list, iter);
		/* release the instance */
		konf_tree_delete((konf_tree_t *)lub_list_node__get_data(iter));
		lub_list_node_free(iter);
	}
	lub_list_free(this->list);

	/* free our memory */
	free(this->line);
	this->line = NULL;
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_new(const char *line, unsigned short priority)
{
	konf_tree_t *this = malloc(sizeof(konf_tree_t));

	if (this)
		konf_tree_init(this, line, priority);

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void konf_tree_delete(konf_tree_t * this)
{
	konf_tree_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
void konf_tree_fprintf(konf_tree_t *this, FILE *stream,
	const char *pattern, int top_depth, int depth,
	bool_t seq, unsigned char prev_pri_hi)
{
	konf_tree_t *conf;
	lub_list_node_t *iter;
	unsigned char pri = 0;
	regex_t regexp;

	if (this->line && (*(this->line) != '\0') &&
		(this->depth > top_depth) &&
		((depth < 0 ) || (this->depth <= (top_depth + depth)))) {
		char *space = NULL;
		unsigned space_num = this->depth - top_depth - 1;
		if (space_num > 0) {
			space = malloc(space_num + 1);
			memset(space, ' ', space_num);
			space[space_num] = '\0';
		}
		if ((0 == this->depth) &&
			(this->splitter ||
			(konf_tree__get_priority_hi(this) != prev_pri_hi)))
			fprintf(stream, "!\n");
		fprintf(stream, "%s", space ? space : "");
		if (seq && (konf_tree__get_seq_num(this) != 0))
			fprintf(stream, "%u ", konf_tree__get_seq_num(this));
		fprintf(stream, "%s\n", this->line);
		free(space);
	}

	/* regexp compilation */
	if (pattern)
		if (regcomp(&regexp, pattern, REG_EXTENDED | REG_ICASE) != 0)
			return;

	/* iterate child elements */
	for(iter = lub_list__get_head(this->list);
		iter; iter = lub_list_node__get_next(iter)) {
		conf = (konf_tree_t *)lub_list_node__get_data(iter);
		if (pattern && (0 != regexec(&regexp, conf->line, 0, NULL, 0)))
			continue;
		konf_tree_fprintf(conf, stream, NULL, top_depth, depth, seq, pri);
		pri = konf_tree__get_priority_hi(conf);
	}
	if (pattern)
		regfree(&regexp);
}

/*-------------------------------------------------------- */
static int normalize_seq(konf_tree_t * this, unsigned short priority,
	lub_list_node_t *start)
{
	unsigned short cnt = 1;
	konf_tree_t *conf = NULL;
	lub_list_node_t *iter;
	unsigned short cur_pri;

	if (start) {
		lub_list_node_t *prev;
		iter = start;
		if ((prev = lub_list_node__get_prev(iter))) {
			conf = (konf_tree_t *)lub_list_node__get_data(prev);
			if (konf_tree__get_priority(conf) == priority)
				cnt = konf_tree__get_seq_num(conf) + 1;
		}
	} else {
		iter = lub_list__get_head(this->list);
	}
	/* If list is empty */
	if (!iter)
		return 0;

	/* Iterate and renum */
	do {
		conf = (konf_tree_t *)lub_list_node__get_data(iter);
		cur_pri = konf_tree__get_priority(conf);
		if (cur_pri > priority)
			break;
		if (cur_pri < priority)
			continue;
		if (konf_tree__get_seq_num(conf) == 0)
			continue;
		konf_tree__set_seq_num(conf, cnt++);
	} while ((iter = lub_list_node__get_next(iter)));

	return 0;
}

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_new_conf(konf_tree_t * this,
	const char *line, unsigned short priority,
	bool_t seq, unsigned short seq_num)
{
	lub_list_node_t *node;
	/* Allocate the memory for a new child element */
	konf_tree_t *newconf = konf_tree_new(line, priority);
	assert(newconf);

	/* Sequence */
	if (seq) {
		konf_tree__set_seq_num(newconf,
			seq_num ? seq_num : 0xffff);
		konf_tree__set_sub_num(newconf, KONF_ENTRY_NEW);
	}

	/* Insert it into the list */
	node = lub_list_add(this->list, newconf);

	if (seq) {
		normalize_seq(this, priority, node);
		konf_tree__set_sub_num(newconf, KONF_ENTRY_OK);
	}

	return newconf;
}

/*--------------------------------------------------------- */
konf_tree_t *konf_tree_find_conf(konf_tree_t * this,
	const char *line, unsigned short priority, unsigned short seq_num)
{
	konf_tree_t *conf;
	lub_list_node_t *iter;
	int check_pri = 0;

	/* If list is empty */
	if (!(iter = lub_list__get_tail(this->list)))
		return NULL;

	if ((0 != priority) && (0 != seq_num))
		check_pri = 1;
	/* Iterate non-empty tree */
	do {
		conf = (konf_tree_t *)lub_list_node__get_data(iter);
		if (check_pri) {
			if (priority < conf->priority)
				continue;
			if (priority > conf->priority)
				break;
			if (seq_num < conf->seq_num)
				continue;
			if (seq_num > conf->seq_num)
				break;
		}
		if (!strcmp(conf->line, line))
			return conf;
	} while ((iter = lub_list_node__get_prev(iter)));

	return NULL;
}

/*--------------------------------------------------------- */
int konf_tree_del_pattern(konf_tree_t *this,
	const char *line, bool_t unique,
	const char *pattern, unsigned short priority,
	bool_t seq, unsigned short seq_num)
{
	int res = 0;
	konf_tree_t *conf;
	lub_list_node_t *iter;
	lub_list_node_t *tmp;
	regex_t regexp;
	int del_cnt = 0; /* how many strings were deleted */

	if (seq && (0 == priority))
		return -1;

	/* Is tree empty? */
	if (!(iter = lub_list__get_head(this->list)))
		return 0;

	/* Compile regular expression */
	if (regcomp(&regexp, pattern, REG_EXTENDED | REG_ICASE) != 0)
		return -1;

	/* Iterate configuration tree */
	tmp = lub_list_node_new(NULL);
	do {
		conf = (konf_tree_t *)lub_list_node__get_data(iter);
		if ((0 != priority) &&
			(priority != conf->priority))
			continue;
		if (seq && (seq_num != 0) &&
			(seq_num != conf->seq_num))
			continue;
		if (seq && (0 == seq_num) && (0 == conf->seq_num))
			continue;
		if (0 != regexec(&regexp, conf->line, 0, NULL, 0))
			continue;
		if (unique && line && !strcmp(conf->line, line)) {
			res++;
			continue;
		}
		lub_list_del(this->list, iter);
		konf_tree_delete(conf);
		lub_list_node_copy(tmp, iter);
		lub_list_node_free(iter);
		iter = tmp;
		del_cnt++;
	} while ((iter = lub_list_node__get_next(iter)));
	lub_list_node_free(tmp);

	regfree(&regexp);

	if (seq && (del_cnt != 0))
		normalize_seq(this, priority, NULL);

	return res;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_priority(const konf_tree_t * this)
{
	return this->priority;
}

/*--------------------------------------------------------- */
unsigned char konf_tree__get_priority_hi(const konf_tree_t * this)
{
	return (unsigned char)(this->priority >> 8);
}

/*--------------------------------------------------------- */
unsigned char konf_tree__get_priority_lo(const konf_tree_t * this)
{
	return (unsigned char)(this->priority & 0xff);
}

/*--------------------------------------------------------- */
bool_t konf_tree__get_splitter(const konf_tree_t * this)
{
	return this->splitter;
}

/*--------------------------------------------------------- */
void konf_tree__set_splitter(konf_tree_t *this, bool_t splitter)
{
	this->splitter = splitter;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_seq_num(const konf_tree_t * this)
{
	return this->seq_num;
}

/*--------------------------------------------------------- */
void konf_tree__set_seq_num(konf_tree_t * this, unsigned short seq_num)
{
	this->seq_num = seq_num;
}

/*--------------------------------------------------------- */
unsigned short konf_tree__get_sub_num(const konf_tree_t * this)
{
	return this->sub_num;
}

/*--------------------------------------------------------- */
void konf_tree__set_sub_num(konf_tree_t * this, unsigned short sub_num)
{
	this->sub_num = sub_num;
}

/*--------------------------------------------------------- */
const char * konf_tree__get_line(const konf_tree_t * this)
{
	return this->line;
}

/*--------------------------------------------------------- */
void konf_tree__set_depth(konf_tree_t * this, int depth)
{
	this->depth = depth;
}

/*--------------------------------------------------------- */
int konf_tree__get_depth(const konf_tree_t * this)
{
	return this->depth;
}
