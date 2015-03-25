/*
 * conf.h
 */
#ifndef _konf_buf_private_h
#define _konf_buf_private_h

#include "konf/buf.h"
#include "lub/bintree.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct konf_buf_s {
	lub_bintree_node_t bt_node;
	int fd;
	int size;
	char *buf;
	int pos;
	int rpos;
	void *data; /* Optional pointer to arbitrary related data */
};

#endif
