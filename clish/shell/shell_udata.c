/*
 * shell_udata.c
 */
#include <assert.h>
#include <string.h>

#include "private.h"
#include "clish/udata.h"

/*-------------------------------------------------------- */
static lub_list_node_t *find_udata_node(const clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;
	clish_udata_t *udata;

	assert(this);
	if (!name)
		return NULL;

	for(iter = lub_list__get_head(this->udata);
		iter; iter = lub_list_node__get_next(iter)) {
		int res;
		udata = (clish_udata_t *)lub_list_node__get_data(iter);
		res = strcmp(clish_udata__get_name(udata), name);
		if (!res)
			return iter;
		if (res > 0) /* No chance to find name */
			break;
	}

	return NULL;
}

/*-------------------------------------------------------- */
static clish_udata_t *find_udata(const clish_shell_t *this, const char *name)
{
	lub_list_node_t *iter;

	if (!(iter = find_udata_node(this, name)))
		return NULL;
	return (clish_udata_t *)lub_list_node__get_data(iter);
}

/*-------------------------------------------------------- */
void *clish_shell__get_udata(const clish_shell_t *this, const char *name)
{
	clish_udata_t *udata;
	assert (this);

	udata = find_udata(this, name);
	return clish_udata__get_data(udata);
}

/*-------------------------------------------------------- */
void *clish_shell__del_udata(clish_shell_t *this, const char *name)
{
	lub_list_node_t *node = NULL;
	clish_udata_t *pdata = NULL;

	if (!this || !name)
		return NULL;

	if(!(node = find_udata_node(this, name)))
		return NULL;

	pdata = (clish_udata_t *)lub_list_node__get_data(node);
	lub_list_del(this->udata, node);
	lub_list_node_free(node);

	return clish_udata_free(pdata);
}

/*-------------------------------------------------------- */
int clish_shell__set_udata(clish_shell_t *this,
	const char *name, void *data)
{
	clish_udata_t *pdata = NULL;

	if (!this || !name)
		return -1;

	if ((pdata = find_udata(this, name))) {
		clish_udata__set_data(pdata, data);
		return 0;
	}

	if (!(pdata = clish_udata_new(name, data)))
		return -1;
	if (lub_list_add(this->udata, pdata))
		return 0;

	clish_udata_free(pdata);
	return -1;
}
/*-------------------------------------------------------- */
