/*
 * config.h
 */
#ifndef _clish_config_h
#define _clish_config_h

#include "lub/types.h"

typedef struct clish_config_s clish_config_t;

/* Possible CONFIG operations */
typedef enum {
	CLISH_CONFIG_NONE,
	CLISH_CONFIG_SET,
	CLISH_CONFIG_UNSET,
	CLISH_CONFIG_DUMP
} clish_config_op_e;

/*=====================================
 * COMMAND INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
clish_config_t *clish_config_new(void);
/*-----------------
 * methods
 *----------------- */
void clish_config_delete(clish_config_t *instance);
void clish_config_dump(const clish_config_t *instance);

/*-----------------
 * attributes
 *----------------- */
void clish_config__set_op(clish_config_t *instance, clish_config_op_e op);
clish_config_op_e clish_config__get_op(const clish_config_t *instance);
void clish_config__set_priority(clish_config_t *instance, unsigned short priority);
unsigned short clish_config__get_priority(const clish_config_t *instance);
void clish_config__set_pattern(clish_config_t *instance, const char *pattern);
char *clish_config__get_pattern(const clish_config_t *instance);
void clish_config__set_file(clish_config_t *instance, const char *file);
char *clish_config__get_file(const clish_config_t *instance);
void clish_config__set_splitter(clish_config_t *instance, bool_t splitter);
bool_t clish_config__get_splitter(const clish_config_t *instance);
void clish_config__set_seq(clish_config_t *instance, const char *seq_num);
const char *clish_config__get_seq(const clish_config_t *instance);
bool_t clish_config__get_unique(const clish_config_t *instance);
void clish_config__set_unique(clish_config_t *instance, bool_t unique);
void clish_config__set_depth(clish_config_t *instance, const char *depth);
const char *clish_config__get_depth(const clish_config_t *instance);

#endif				/* _clish_config_h */
