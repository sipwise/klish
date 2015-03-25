/*
 * ini.h
 */

#ifndef _lub_ini_h
#define _lub_ini_h

#include "lub/c_decl.h"
#include "lub/list.h"

typedef struct lub_pair_s lub_pair_t;
typedef struct lub_ini_s lub_ini_t;
typedef lub_list_node_t lub_ini_node_t;

_BEGIN_C_DECL

/* Pair */
int lub_pair_compare(const void *first, const void *second);
void lub_pair_init(lub_pair_t *instance, const char *name, const char *value);
lub_pair_t *lub_pair_new(const char *name, const char *value);
void lub_pair_fini(lub_pair_t *instance);
void lub_pair_free(lub_pair_t *instance);
const char *lub_pair__get_name(const lub_pair_t *instance);
void lub_pair__set_name(lub_pair_t *instance, const char *name);
const char *lub_pair__get_value(const lub_pair_t *instance);
void lub_pair__set_value(lub_pair_t *instance, const char *value);

/* Ini */
void lub_ini_init(lub_ini_t *instance);
lub_ini_t *lub_ini_new(void);
void lub_ini_fini(lub_ini_t *instance);
void lub_ini_free(lub_ini_t *instance);
lub_pair_t *lub_ini_find_pair(const lub_ini_t *instance, const char *name);
const char *lub_ini_find(const lub_ini_t *instance, const char *name);
void lub_ini_add(lub_ini_t *instance, lub_pair_t *pair);
int lub_ini_parse_str(lub_ini_t *instance, const char *ini);
int lub_ini_parse_file(lub_ini_t *instance, const char *fn);
lub_ini_node_t *lub_ini__get_head(lub_ini_t *instance);
lub_ini_node_t *lub_ini__get_tail(lub_ini_t *instance);
lub_ini_node_t *lub_ini__get_next(lub_ini_node_t *node);
lub_ini_node_t *lub_ini__get_prev(lub_ini_node_t *node);
lub_pair_t *lub_ini__iter_data(lub_ini_node_t *node);

_END_C_DECL

#endif				/* _lub_ini_h */
