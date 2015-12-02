#ifndef _konf_query_private_h
#define _konf_query_private_h

#include "konf/query.h"
#include "lub/types.h"

struct konf_query_s {
	konf_query_op_e op;
	char *pattern;
	unsigned short priority;
	bool_t seq; /* sequence aka auto priority */
	unsigned short seq_num; /* sequence number */
	unsigned int pwdc;
	char **pwd;
	char *line;
	char *path;
	bool_t splitter;
	bool_t unique;
	int depth;
};

#endif
