/*
 * types.h
 */

#ifndef _clish_types_h
#define _clish_types_h

#include "lub/c_decl.h"
#include "lub/argv.h"

struct clish_help_s {
	lub_argv_t *name;
	lub_argv_t *help;
	lub_argv_t *detail;
};
typedef struct clish_help_s clish_help_t;

#endif /* _clish_types_h */
