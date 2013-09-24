/*
 * ptype.h
 */
#include "clish/pargv.h"
#include "lub/bintree.h"
#include "lub/argv.h"

#include <sys/types.h>
#include <regex.h>

typedef struct clish_ptype_integer_s clish_ptype_integer_t;
struct clish_ptype_integer_s {
	int min;
	int max;
};

typedef struct clish_ptype_select_s clish_ptype_select_t;
struct clish_ptype_select_s {
	lub_argv_t *items;
};

struct clish_ptype_s {
	lub_bintree_node_t bt_node;
	char *name;
	char *text;
	char *pattern;
	char *range;
	clish_ptype_method_e method;
	clish_ptype_preprocess_e preprocess;
	unsigned last_name;	/* index used for auto-completion */
	union {
		regex_t regexp;
		clish_ptype_integer_t integer;
		clish_ptype_select_t select;
	} u;
};
