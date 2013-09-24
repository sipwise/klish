/*
 * pargv.h
 */
#include "clish/pargv.h"
#include "clish/param.h"

/*--------------------------------------------------------- */
struct clish_parg_s {
	const clish_param_t *param;
	char *value;
};
struct clish_pargv_s {
	unsigned pargc;
	clish_parg_t **pargv;
};
/*--------------------------------------------------------- */
