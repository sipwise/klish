/*
 * nspace.h
 */
/**
\ingroup clish
\defgroup clish_nspace nspace
@{

\brief This class represents an instance of a namespace type.

Namespace instances are assocated with a view to make view's commands available
within current view.

*/
#ifndef _clish_nspace_h
#define _clish_nspace_h

typedef struct clish_nspace_s clish_nspace_t;

typedef enum {
	CLISH_NSPACE_NONE,
	CLISH_NSPACE_HELP,
	CLISH_NSPACE_COMPLETION,
	CLISH_NSPACE_CHELP
} clish_nspace_visibility_e;

#include <regex.h>

#include "clish/view.h"

/*=====================================
 * NSPACE INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_nspace_t *clish_nspace_new(const char *view_name);
/*-----------------
 * methods
 *----------------- */
void clish_nspace_delete(clish_nspace_t * instance);
const clish_command_t *clish_nspace_find_next_completion(clish_nspace_t *
	instance, const char *iter_cmd, const char *line,
	clish_nspace_visibility_e field);
clish_command_t *clish_nspace_find_command(clish_nspace_t * instance, const char *name);
void clish_nspace_dump(const clish_nspace_t * instance);
clish_command_t * clish_nspace_create_prefix_cmd(clish_nspace_t * instance,
	const char * name, const char * help);
void clish_nspace_clean_proxy(clish_nspace_t * instance);
/*-----------------
 * attributes
 *----------------- */
void clish_nspace__set_view(clish_nspace_t *instance, clish_view_t *view);
clish_view_t *clish_nspace__get_view(const clish_nspace_t * instance);
void clish_nspace__set_view_name(clish_nspace_t *instance, const char *view_name);
const char * clish_nspace__get_view_name(const clish_nspace_t *instance);
const char *clish_nspace__get_prefix(const clish_nspace_t * instance);
const regex_t *clish_nspace__get_prefix_regex(const clish_nspace_t * instance);
bool_t clish_nspace__get_help(const clish_nspace_t * instance);
bool_t clish_nspace__get_completion(const clish_nspace_t * instance);
bool_t clish_nspace__get_context_help(const clish_nspace_t * instance);
bool_t clish_nspace__get_inherit(const clish_nspace_t * instance);
bool_t clish_nspace__get_visibility(const clish_nspace_t * instance,
	clish_nspace_visibility_e field);
void clish_nspace__set_prefix(clish_nspace_t * instance, const char *prefix);
void clish_nspace__set_help(clish_nspace_t * instance, bool_t help);
void clish_nspace__set_completion(clish_nspace_t * instance, bool_t help);
void clish_nspace__set_context_help(clish_nspace_t * instance, bool_t help);
void clish_nspace__set_inherit(clish_nspace_t * instance, bool_t inherit);
void clish_nspace__set_access(clish_nspace_t *instance, const char *access);
char *clish_nspace__get_access(const clish_nspace_t *instance);

#endif				/* _clish_nspace_h */
/** @} clish_nspace */
