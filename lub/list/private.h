#include "lub/list.h"

struct lub_list_node_s {
	lub_list_node_t *prev;
	lub_list_node_t *next;
	void *data;
};

struct lub_list_s {
	lub_list_node_t *head;
	lub_list_node_t *tail;
	lub_list_compare_fn *compareFn;
	unsigned int len;
};
