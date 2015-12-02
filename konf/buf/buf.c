/*
 * buf.c
 *
 * This file provides the implementation of a buf class
 */
#include "private.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define KONF_BUF_CHUNK 1024

/*---------------------------------------------------------
 * PRIVATE META FUNCTIONS
 *--------------------------------------------------------- */
int konf_buf_bt_compare(const void *clientnode, const void *clientkey)
{
	const konf_buf_t *this = clientnode;
	int keyfd;

	memcpy(&keyfd, clientkey, sizeof(keyfd));

	return (this->fd - keyfd);
}

/*-------------------------------------------------------- */
static void konf_buf_key(lub_bintree_key_t * key,
	int fd)
{
	memcpy(key, &fd, sizeof(fd));
}

/*-------------------------------------------------------- */
void konf_buf_bt_getkey(const void *clientnode, lub_bintree_key_t * key)
{
	const konf_buf_t *this = clientnode;

	konf_buf_key(key, this->fd);
}

/*---------------------------------------------------------
 * PRIVATE METHODS
 *--------------------------------------------------------- */
static void
konf_buf_init(konf_buf_t * this, int fd)
{
	this->fd = fd;
	this->buf = malloc(KONF_BUF_CHUNK);
	this->size = KONF_BUF_CHUNK;
	this->pos = 0;
	this->rpos = 0;
	this->data = NULL;

	/* Be a good binary tree citizen */
	lub_bintree_node_init(&this->bt_node);
}

/*--------------------------------------------------------- */
static void konf_buf_fini(konf_buf_t * this)
{
	free(this->buf);
}

/*---------------------------------------------------------
 * PUBLIC META FUNCTIONS
 *--------------------------------------------------------- */
size_t konf_buf_bt_offset(void)
{
	return offsetof(konf_buf_t, bt_node);
}

/*--------------------------------------------------------- */
konf_buf_t *konf_buf_new(int fd)
{
	konf_buf_t *this = malloc(sizeof(konf_buf_t));

	if (this)
		konf_buf_init(this, fd);

	return this;
}

/*---------------------------------------------------------
 * PUBLIC METHODS
 *--------------------------------------------------------- */
void konf_buf_delete(konf_buf_t * this)
{
	konf_buf_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
static int konf_buf_realloc(konf_buf_t *this, int addsize)
{
	int chunk = KONF_BUF_CHUNK;
	char *tmpbuf;

	if (addsize > chunk)
		chunk = addsize;
	if ((this->size - this->pos) < chunk) {
		tmpbuf = realloc(this->buf, this->size + chunk);
		this->buf = tmpbuf;
		this->size += chunk;
	}

	return this->size;
}

/*--------------------------------------------------------- */
int konf_buf_add(konf_buf_t *this, void *str, size_t len)
{
	char *buffer;

	konf_buf_realloc(this, len);
	buffer = this->buf + this->pos;
	memcpy(buffer, str, len);
	this->pos += len;

	return len;
}

/*--------------------------------------------------------- */
int konf_buf_read(konf_buf_t *this)
{
	char *buffer;
	int buffer_size;
	int nbytes;

	konf_buf_realloc(this, 0);
	buffer_size = this->size - this->pos;
	buffer = this->buf + this->pos;

	nbytes = read(this->fd, buffer, buffer_size);
	if (nbytes > 0)
		this->pos += nbytes;

	return nbytes;
}

/*--------------------------------------------------------- */
char * konf_buf_string(char *buf, int len)
{
	int i;
	char *str;

	for (i = 0; i < len; i++) {
		if (('\0' == buf[i]) ||
			('\n' == buf[i]))
			break;
	}
	if (i >= len)
		return NULL;

	str = malloc(i + 1);
	memcpy(str, buf, i + 1);
	str[i] = '\0';

	return str;
}

/*--------------------------------------------------------- */
char * konf_buf_parse(konf_buf_t *this)
{
	char * str = NULL;

	/* Search the buffer for the string */
	str = konf_buf_string(this->buf, this->pos);

	/* Remove parsed string from the buffer */
	if (str) {
		int len = strlen(str) + 1;
		memmove(this->buf, &this->buf[len], this->pos - len);
		this->pos -= len;
		if (this->rpos >= len)
			this->rpos -= len;
		else
			this->rpos = 0;
	}

	/* Make buffer shorter */
	if ((this->size - this->pos) > (2 * KONF_BUF_CHUNK)) {
		char *tmpbuf;
		tmpbuf = realloc(this->buf, this->size - KONF_BUF_CHUNK);
		this->buf = tmpbuf;
		this->size -= KONF_BUF_CHUNK;
	}

	return str;
}

/*--------------------------------------------------------- */
char * konf_buf_preparse(konf_buf_t *this)
{
	char * str = NULL;

	str = konf_buf_string(this->buf + this->rpos, this->pos - this->rpos);
	if (str)
		this->rpos += (strlen(str) + 1);

	return str;
}

/*--------------------------------------------------------- */
int konf_buf_lseek(konf_buf_t *this, int newpos)
{
	if (newpos > this->pos)
		return -1;
	this->rpos = newpos;

	return newpos;
}

/*--------------------------------------------------------- */
int konf_buf__get_fd(const konf_buf_t * this)
{
	return this->fd;
}

/*--------------------------------------------------------- */
int konf_buf__get_len(const konf_buf_t *this)
{
	return this->pos;
}

/*--------------------------------------------------------- */
char * konf_buf__dup_line(const konf_buf_t *this)
{
	char *str;

	str = malloc(this->pos + 1);
	memcpy(str, this->buf, this->pos);
	str[this->pos] = '\0';
	return str;
}

/*--------------------------------------------------------- */
char * konf_buf__get_buf(const konf_buf_t *this)
{
	return this->buf;
}

/*--------------------------------------------------------- */
void * konf_buf__get_data(const konf_buf_t *this)
{
	return this->data;
}

/*--------------------------------------------------------- */
void konf_buf__set_data(konf_buf_t *this, void *data)
{
	this->data = data;
}

/*---------------------------------------------------------
 * buftree functions
 *--------------------------------------------------------- */

/*--------------------------------------------------------- */
konf_buf_t *konf_buftree_find(lub_bintree_t * this,
	int fd)
{
	lub_bintree_key_t key;

	konf_buf_key(&key, fd);

	return lub_bintree_find(this, &key);
}

/*--------------------------------------------------------- */
void konf_buftree_remove(lub_bintree_t * this,
	int fd)
{
	konf_buf_t *tbuf;

	if ((tbuf = konf_buftree_find(this, fd)) == NULL)
		return;

	lub_bintree_remove(this, tbuf);
	konf_buf_delete(tbuf);
}

/*--------------------------------------------------------- */
int konf_buftree_read(lub_bintree_t * this, int fd)
{
	konf_buf_t *buf;

	buf = konf_buftree_find(this, fd);
	if (!buf)
		return -1;

	return konf_buf_read(buf);
}


/*--------------------------------------------------------- */
char * konf_buftree_parse(lub_bintree_t * this,
	int fd)
{
	konf_buf_t *buf;

	buf = konf_buftree_find(this, fd);
	if (!buf)
		return NULL;

	return konf_buf_parse(buf);
}

