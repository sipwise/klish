/*
 * view.h
 */
 /**
\ingroup clish
\defgroup clish_view view
@{

\brief This class is a container of commands.

A particular CLI session may contain a number of different views. Each 
view may contain its own specific commands as well as those available at
a global scope.

*/
#ifndef _clish_view_h
#define _clish_view_h

typedef struct clish_view_s clish_view_t;

typedef enum {
	CLISH_RESTORE_NONE,
	CLISH_RESTORE_DEPTH,
	CLISH_RESTORE_VIEW
} clish_view_restore_e;

#include "lub/list.h"
#include "clish/command.h"
#include "clish/nspace.h"
#include "clish/var.h"

/*=====================================
 * VIEW INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_view_t *clish_view_new(const char *name, const char *prompt);
int clish_view_bt_compare(const void *clientnode, const void *clientkey);
void clish_view_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_view_bt_offset(void);

/*-----------------
 * methods
 *----------------- */
lub_bintree_t * clish_view__get_command_tree(clish_view_t *instance);
lub_list_t * clish_view__get_nspace_tree(clish_view_t *instance);
void clish_view_delete(clish_view_t * instance);
clish_command_t *clish_view_new_command(clish_view_t * instance,
	const char *name, const char *text);
clish_command_t *clish_view_find_command(clish_view_t * instance,
	const char *name, bool_t inherit);
const clish_command_t *clish_view_find_next_completion(clish_view_t * instance,
	const char *iter_cmd, const char *line,
	clish_nspace_visibility_e field, bool_t inherit);
clish_command_t *clish_view_resolve_command(clish_view_t * instance,
	const char *line, bool_t inherit);
clish_command_t *clish_view_resolve_prefix(clish_view_t * instance,
	const char *line, bool_t inherit);
void clish_view_dump(clish_view_t * instance);
void clish_view_insert_nspace(clish_view_t * instance, clish_nspace_t * nspace);
void clish_view_clean_proxy(clish_view_t * instance);

/*-----------------
 * attributes
 *----------------- */
const char *clish_view__get_name(const clish_view_t * instance);
void clish_view__set_prompt(clish_view_t * instance, const char *prompt);
char *clish_view__get_prompt(const clish_view_t *instance);
void clish_view__set_depth(clish_view_t * instance, unsigned depth);
unsigned clish_view__get_depth(const clish_view_t * instance);
void clish_view__set_restore(clish_view_t * instance,
	clish_view_restore_e restore);
clish_view_restore_e clish_view__get_restore(const clish_view_t * instance);
int clish_view_insert_hotkey(const clish_view_t *instance, const char *key, const char *cmd);
const char *clish_view_find_hotkey(const clish_view_t *instance, int code);
void clish_view__set_access(clish_view_t *instance, const char *access);
char *clish_view__get_access(const clish_view_t *instance);

#endif				/* _clish_view_h */
/** @} clish_view */
