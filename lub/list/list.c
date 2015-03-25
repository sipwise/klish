#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "private.h"

/*--------------------------------------------------------- */
static inline void lub_list_init(lub_list_t * this,
	lub_list_compare_fn compareFn)
{
	this->head = NULL;
	this->tail = NULL;
	this->compareFn = compareFn;
	this->len = 0;
}

/*--------------------------------------------------------- */
lub_list_t *lub_list_new(lub_list_compare_fn compareFn)
{
	lub_list_t *this;

	this = malloc(sizeof(*this));
	assert(this);
	lub_list_init(this, compareFn);

	return this;
}

/*--------------------------------------------------------- */
inline void lub_list_free(lub_list_t *this)
{
	free(this);
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list__get_head(lub_list_t *this)
{
	return this->head;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list__get_tail(lub_list_t *this)
{
	return this->tail;
}

/*--------------------------------------------------------- */
static inline void lub_list_node_init(lub_list_node_t *this,
	void *data)
{
	this->prev = this->next = NULL;
	this->data = data;
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_node_new(void *data)
{
	lub_list_node_t *this;

	this = malloc(sizeof(*this));
	assert(this);
	lub_list_node_init(this, data);

	return this;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_init(lub_list_t *this)
{
	return this->head;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_node__get_prev(lub_list_node_t *this)
{
	return this->prev;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_node__get_next(lub_list_node_t *this)
{
	return this->next;
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_next(lub_list_node_t *this)
{
	return lub_list_node__get_next(this);
}

/*--------------------------------------------------------- */
inline lub_list_node_t *lub_list_iterator_prev(lub_list_node_t *this)
{
	return lub_list_node__get_prev(this);
}

/*--------------------------------------------------------- */
inline void lub_list_node_free(lub_list_node_t *this)
{
	free(this);
}

/*--------------------------------------------------------- */
inline void *lub_list_node__get_data(lub_list_node_t *this)
{
	return this->data;
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_add(lub_list_t *this, void *data)
{
	lub_list_node_t *node = lub_list_node_new(data);
	lub_list_node_t *iter;

	this->len++;

	/* Empty list */
	if (!this->head) {
		this->head = node;
		this->tail = node;
		return node;
	}

	/* Not sorted list. Add to the tail. */
	if (!this->compareFn) {
		node->prev = this->tail;
		node->next = NULL;
		this->tail->next = node;
		this->tail = node;
		return node;
	}

	/* Sorted list */
	iter = this->tail;
	while (iter) {
		if (this->compareFn(node->data, iter->data) >= 0) {
			node->next = iter->next;
			node->prev = iter;
			iter->next = node;
			if (node->next)
				node->next->prev = node;
			break;
		}
		iter = iter->prev;
	}
	/* Insert node into the list head */
	if (!iter) {
		node->next = this->head;
		node->prev = NULL;
		this->head->prev = node;
		this->head = node;
	}
	if (!node->next)
		this->tail = node;

	return node;
}

/*--------------------------------------------------------- */
void lub_list_del(lub_list_t *this, lub_list_node_t *node)
{
	if (node->prev)
		node->prev->next = node->next;
	else
		this->head = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		this->tail = node->prev;

	this->len--;
}

/*--------------------------------------------------------- */
inline void lub_list_node_copy(lub_list_node_t *dst, lub_list_node_t *src)
{
	memcpy(dst, src, sizeof(lub_list_node_t));
}

/*--------------------------------------------------------- */
lub_list_node_t *lub_list_search(lub_list_t *this, void *data)
{
	lub_list_node_t *iter;

	/* Empty list */
	if (!this->head)
		return NULL;
	/* Not sorted list. Can't search. */
	if (!this->compareFn)
		return NULL;

	/* Sorted list */
	iter = this->head;
	while (iter) {
		if (!this->compareFn(data, iter->data))
			return iter;
		iter = iter->next;
	}

	return NULL;
}

/*--------------------------------------------------------- */
inline unsigned int lub_list_len(lub_list_t *this)
{
	return this->len;
}

/*--------------------------------------------------------- */
