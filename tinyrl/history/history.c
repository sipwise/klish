/*
 * history.c
 * 
 * Simple non-readline hooks for the cli library
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "private.h"
#include "lub/string.h"
#include "tinyrl/history.h"

struct _tinyrl_history {
	tinyrl_history_entry_t **entries;	/* pointer entries */
	unsigned length;	/* Number of elements within this array */
	unsigned size;		/* Number of slots allocated in this array */
	unsigned current_index;
	unsigned stifle;
};

/*------------------------------------- */
void tinyrl_history_init(tinyrl_history_t * this, unsigned stifle)
{
	this->entries = NULL;
	this->stifle = stifle;
	this->current_index = 1;
	this->length = 0;
	this->size = 0;
}

/*------------------------------------- */
void tinyrl_history_fini(tinyrl_history_t * this)
{
	tinyrl_history_entry_t *entry;
	tinyrl_history_iterator_t iter;

	/* release the resource associated with each entry */
	for (entry = tinyrl_history_getfirst(this, &iter);
	     entry; entry = tinyrl_history_getnext(&iter)) {
		tinyrl_history_entry_delete(entry);
	}
	/* release the list */
	free(this->entries);
	this->entries = NULL;
}

/*------------------------------------- */
tinyrl_history_t *tinyrl_history_new(unsigned stifle)
{
	tinyrl_history_t *this = malloc(sizeof(tinyrl_history_t));
	if (NULL != this) {
		tinyrl_history_init(this, stifle);
	}
	return this;
}

/*------------------------------------- */
void tinyrl_history_delete(tinyrl_history_t * this)
{
	tinyrl_history_fini(this);
	free(this);
}

/*
HISTORY LIST MANAGEMENT 
*/
/*------------------------------------- */
/* insert a new entry at the current offset */
static void insert_entry(tinyrl_history_t * this, const char *line)
{
	tinyrl_history_entry_t *new_entry =
	    tinyrl_history_entry_new(line, this->current_index++);
	assert(this->length);
	assert(this->entries);
	if (new_entry) {
		this->entries[this->length - 1] = new_entry;
	}
}

/*------------------------------------- */
/*
 * This frees the specified entries from the 
 * entries vector. NB it doesn't perform any shuffling.
 * This function is inclusive of start and end
 */
static void
free_entries(const tinyrl_history_t * this, unsigned start, unsigned end)
{
	unsigned i;
	assert(start <= end);
	assert(end < this->length);

	for (i = start; i <= end; i++) {
		tinyrl_history_entry_t *entry = this->entries[i];
		tinyrl_history_entry_delete(entry);
		entry = NULL;
	}
}

/*------------------------------------- */
/*
 * This removes the specified entries from the 
 * entries vector. Shuffling up the array as necessary 
 * This function is inclusive of start and end
 */
static void
remove_entries(tinyrl_history_t * this, unsigned start, unsigned end)
{
	unsigned delta = (end - start) + 1;	/* number of entries being deleted */
	/* number of entries to shuffle */
	unsigned num_entries = (this->length - end) - 1;
	assert(start <= end);
	assert(end < this->length);

	if (num_entries) {
		/* move the remaining entries down to close the array */
		memmove(&this->entries[start],
			&this->entries[end + 1],
			sizeof(tinyrl_history_entry_t *) * num_entries);
	}
	/* now fix up the length variables */
	this->length -= delta;
}

/*------------------------------------- */
/* 
Search the current history buffer for the specified 
line and if found remove it.
*/
static bool_t remove_duplicate(tinyrl_history_t * this, const char *line)
{
	bool_t result = BOOL_FALSE;
	unsigned i;

	for (i = 0; i < this->length; i++) {
		tinyrl_history_entry_t *entry = this->entries[i];
		if (0 == strcmp(line, tinyrl_history_entry__get_line(entry))) {
			free_entries(this, i, i);
			remove_entries(this, i, i);
			result = BOOL_TRUE;
			break;
		}
	}
	return result;
}

/*------------------------------------- */
/* 
add an entry to the end of the current array 
if there is no space returns -1 else 0
*/
static void append_entry(tinyrl_history_t * this, const char *line)
{
	if (this->length < this->size) {
		this->length++;

		insert_entry(this, line);
	}
}

/*------------------------------------- */
/*
 add a new history entry replacing the oldest one 
 */
static void add_n_replace(tinyrl_history_t * this, const char *line)
{
	if (BOOL_FALSE == remove_duplicate(this, line)) {
		/* free the oldest entry */
		free_entries(this, 0, 0);
		/* shuffle the array */
		remove_entries(this, 0, 0);
	}
	/* add the new entry */
	append_entry(this, line);
}

/*------------------------------------- */
/* add a new history entry growing the array if necessary */
static void add_n_grow(tinyrl_history_t * this, const char *line)
{
	if (this->size == this->length) {
		/* increment the history memory by 10 entries each time we grow */
		unsigned new_size = this->size + 10;
		size_t nbytes;
		tinyrl_history_entry_t **new_entries;

		nbytes = sizeof(tinyrl_history_entry_t *) * new_size;
		new_entries = realloc(this->entries, nbytes);
		if (NULL != new_entries) {
			this->size = new_size;
			this->entries = new_entries;
		}
	}
	(void)remove_duplicate(this, line);
	append_entry(this, line);
}

/*------------------------------------- */
void tinyrl_history_add(tinyrl_history_t * this, const char *line)
{
	if (this->length && (this->length == this->stifle)) {
		add_n_replace(this, line);
	} else {
		add_n_grow(this, line);
	}
}

/*------------------------------------- */
tinyrl_history_entry_t *tinyrl_history_remove(tinyrl_history_t * this,
					      unsigned offset)
{
	tinyrl_history_entry_t *result = NULL;

	if (offset < this->length) {
		result = this->entries[offset];
		/* do the biz */
		remove_entries(this, offset, offset);
	}
	return result;
}

/*------------------------------------- */
void tinyrl_history_clear(tinyrl_history_t * this)
{
	/* free all the entries */
	free_entries(this, 0, this->length - 1);
	/* and shuffle the array */
	remove_entries(this, 0, this->length - 1);
}

/*------------------------------------- */
void tinyrl_history_stifle(tinyrl_history_t * this, unsigned stifle)
{
	/* 
	 * if we are stifling (i.e. non zero value) then 
	 * delete the obsolete entries
	 */
	if (stifle) {
		if (stifle < this->length) {
			unsigned num_deletes = this->length - stifle;
			/* free the entries */
			free_entries(this, 0, num_deletes - 1);
			/* shuffle the array shut */
			remove_entries(this, 0, num_deletes - 1);
		}
		this->stifle = stifle;
	}
}

/*------------------------------------- */
unsigned tinyrl_history_unstifle(tinyrl_history_t * this)
{
	unsigned result = this->stifle;

	this->stifle = 0;

	return result;
}

/*------------------------------------- */
bool_t tinyrl_history_is_stifled(const tinyrl_history_t * this)
{
	return this->stifle ? BOOL_TRUE : BOOL_FALSE;
}

/*
INFORMATION ABOUT THE HISTORY LIST 
*/
tinyrl_history_entry_t *tinyrl_history_get(const tinyrl_history_t * this,
					   unsigned position)
{
	unsigned i;
	tinyrl_history_entry_t *entry = NULL;
	for (i = 0; i < this->length; i++) {
		entry = this->entries[i];
		if (position == tinyrl_history_entry__get_index(entry)) {
			/* found it */
			break;
		}
		entry = NULL;
	}
	return entry;
}

/*------------------------------------- */
tinyrl_history_expand_t
tinyrl_history_expand(const tinyrl_history_t * this,
		      const char *string, char **output)
{
	tinyrl_history_expand_t result = tinyrl_history_NO_EXPANSION;	/* no expansion */
	const char *p, *start;
	char *buffer = NULL;
	unsigned len;

	for (p = string, start = string, len = 0; *p; p++, len++) {
		/* perform pling substitution */
		if (*p == '!') {
			/* assume the last command to start with... */
			unsigned offset = this->current_index - 1;
			unsigned skip;
			tinyrl_history_entry_t *entry;

			/* this could be an escape sequence */
			if (p[1] != '!') {
				int tmp;
				int res;
				/* read the numeric identifier */
				res = sscanf(p, "!%d", &tmp);
				if ((0 == res) || (EOF == res)) {
					/* error so ignore it */
					break;
				}

				if (tmp < 0) {
					/* this is a relative reference */
					/*lint -e737 Loss of sign in promotion from int to unsigend int */
					offset += tmp;	/* adding a negative substracts... */
					/*lint +e737 */
				} else {
					/* this is an absolute reference */
					offset = (unsigned)tmp;
				}
			}
			if (len > 0) {
				/* we need to add in some previous plain text */
				lub_string_catn(&buffer, start, len);
			}

			/* skip the escaped chars */
			p += skip = strspn(p, "!-0123456789");

			/* try and find the history entry */
			entry = tinyrl_history_get(this, offset);
			if (NULL != entry) {
				/* reset the non-escaped references */
				start = p;
				len = 0;
				/* add the expanded text to the buffer */
				result = tinyrl_history_EXPANDED;
				lub_string_cat(&buffer,
					       tinyrl_history_entry__get_line
					       (entry));
			} else {
				/* we simply leave the unexpanded sequence */
				len += skip;
			}
		}
	}
	/* add any left over plain text */
	lub_string_catn(&buffer, start, len);
	*output = buffer;

	return result;
}

/*-------------------------------------*/
tinyrl_history_entry_t *tinyrl_history_getfirst(const tinyrl_history_t * this,
						tinyrl_history_iterator_t *
						iter)
{
	tinyrl_history_entry_t *result = NULL;

	iter->history = this;
	iter->offset = 0;

	if (this->length) {
		result = this->entries[iter->offset];
	}
	return result;
}

/*-------------------------------------*/
tinyrl_history_entry_t *tinyrl_history_getnext(tinyrl_history_iterator_t * iter)
{
	tinyrl_history_entry_t *result = NULL;

	if (iter->offset < iter->history->length - 1) {
		iter->offset++;
		result = iter->history->entries[iter->offset];
	}

	return result;
}

/*-------------------------------------*/
tinyrl_history_entry_t *tinyrl_history_getlast(const tinyrl_history_t * this,
					       tinyrl_history_iterator_t * iter)
{
	iter->history = this;
	iter->offset = this->length;

	return tinyrl_history_getprevious(iter);
}

/*-------------------------------------*/
tinyrl_history_entry_t *tinyrl_history_getprevious(tinyrl_history_iterator_t *
						   iter)
{
	tinyrl_history_entry_t *result = NULL;

	if (iter->offset) {
		iter->offset--;
		result = iter->history->entries[iter->offset];
	}

	return result;
}

/*-------------------------------------*/
/* Save command history to specified file */
int tinyrl_history_save(const tinyrl_history_t *this, const char *fname)
{
	tinyrl_history_entry_t *entry;
	tinyrl_history_iterator_t iter;
	FILE *f;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}
	if (!(f = fopen(fname, "w")))
		return -1;
	for (entry = tinyrl_history_getfirst(this, &iter);
		entry; entry = tinyrl_history_getnext(&iter)) {
		if (fprintf(f, "%s\n", tinyrl_history_entry__get_line(entry)) < 0)
			return -1;
	}
	fclose(f);

	return 0;
}

/*-------------------------------------*/
/* Restore command history from specified file */
int tinyrl_history_restore(tinyrl_history_t *this, const char *fname)
{
	FILE *f;
	char *p;
	int part_len = 300;
	char *buf;
	int buf_len = part_len;
	int res = 0;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}
	if (!(f = fopen(fname, "r")))
		return 0; /* Can't find history file */

	buf = malloc(buf_len);
	p = buf;
	while (fgets(p, buf_len - (p - buf), f)) {
		char *ptmp = NULL;
		char *el = strchr(buf, '\n');
		if (el) { /* The whole line was readed */
			*el = '\0';
			tinyrl_history_add(this, buf);
			p = buf;
			continue;
		}
		buf_len += part_len;
		ptmp = realloc(buf, buf_len);
		if (!ptmp) {
			res = -1;
			goto end;
		}
		buf = ptmp;
		p = buf + buf_len - part_len - 1;
	}
end:
	free(buf);
	fclose(f);

	return res;
}


/*-------------------------------------*/
