/*
 * clish/config/private.h
 */

#include "clish/config.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */
struct clish_config_s {
	clish_config_op_e op; /* CONFIG operation */
	unsigned short priority;
	char *pattern;
	char *file;
	bool_t splitter;
	char *seq;
	bool_t unique;
	char *depth;
};
