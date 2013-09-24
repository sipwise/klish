/*
 * pargv.h
 */
 /**
\ingroup clish
\defgroup clish_pargv pargv
@{

\brief This class represents a vector of command line arguments.

*/

#ifndef _clish_pargv_h
#define _clish_pargv_h

typedef enum {
	CLISH_LINE_OK,
	CLISH_LINE_PARTIAL,
	CLISH_BAD_CMD,
	CLISH_BAD_PARAM,
	CLISH_BAD_HISTORY
} clish_pargv_status_t;

typedef struct clish_pargv_s clish_pargv_t;
typedef struct clish_parg_s clish_parg_t;

#include "clish/ptype.h"
#include "clish/command.h"
#include "clish/param.h"

/*=====================================
 * PARGV INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_pargv_t *clish_pargv_new(void);
/*-----------------
 * methods
 *----------------- */
void clish_pargv_delete(clish_pargv_t * instance);
const clish_parg_t *clish_pargv_find_arg(clish_pargv_t * instance,
	const char *name);
int clish_pargv_insert(clish_pargv_t * instance,
	const clish_param_t * param, const char *value);
void clish_pargv_dump(const clish_pargv_t * instance);
/*-----------------
 * attributes
 *----------------- */
unsigned clish_pargv__get_count(clish_pargv_t * instance);
clish_parg_t *clish_pargv__get_parg(clish_pargv_t * instance, unsigned index);
const clish_param_t *clish_pargv__get_param(clish_pargv_t * instance,
	unsigned index);

/*=====================================
 * PARG INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
/*-----------------
 * methods
 *----------------- */
void clish_parg_dump(const clish_parg_t * instance);

/*-----------------
 * attributes 
 *----------------- */
const char *clish_parg__get_name(const clish_parg_t * instance);
const char *clish_parg__get_value(const clish_parg_t * instance);
const clish_ptype_t *clish_parg__get_ptype(const clish_parg_t * instance);

#endif				/* _clish_pargv_h */
/** @} clish_pargv */
