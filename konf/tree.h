/*
 * conf.h
 */
 /**
\ingroup clish
\defgroup clish_conf config
@{

\brief This class is a config in memory container.

Use it to implement config in memory.

*/
#ifndef _konf_tree_h
#define _konf_tree_h

#include <stdio.h>

#include "lub/types.h"
#include "lub/list.h"

typedef struct konf_tree_s konf_tree_t;

#define KONF_ENTRY_OK 0xffff
#define KONF_ENTRY_DIRTY 0xfffe
#define KONF_ENTRY_NEW 0xfffd

/*=====================================
 * CONF INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
konf_tree_t *konf_tree_new(const char *line, unsigned short priority);

/*-----------------
 * methods
 *----------------- */
void konf_tree_delete(konf_tree_t * instance);
void konf_tree_fprintf(konf_tree_t * instance, FILE * stream,
	const char *pattern, int top_depth, int depth,
	bool_t seq, unsigned char prev_pri_hi);
konf_tree_t *konf_tree_new_conf(konf_tree_t * instance,
	const char *line, unsigned short priority,
	bool_t seq, unsigned short seq_num);
konf_tree_t *konf_tree_find_conf(konf_tree_t * instance,
	const char *line, unsigned short priority, unsigned short sequence);
int konf_tree_del_pattern(konf_tree_t * instance,
	const char *line, bool_t unique,
	const char *pattern, unsigned short priority,
	bool_t seq, unsigned short seq_num);

/*-----------------
 * attributes
 *----------------- */
unsigned short konf_tree__get_priority(const konf_tree_t * instance);
unsigned char konf_tree__get_priority_hi(const konf_tree_t * instance);
unsigned char konf_tree__get_priority_lo(const konf_tree_t * instance);
bool_t konf_tree__get_splitter(const konf_tree_t * instance);
void konf_tree__set_splitter(konf_tree_t *instance, bool_t splitter);
unsigned short konf_tree__get_seq_num(const konf_tree_t * instance);
void konf_tree__set_seq_num(konf_tree_t * instance, unsigned short seq_num);
unsigned short konf_tree__get_sub_num(const konf_tree_t * instance);
void konf_tree__set_sub_num(konf_tree_t * instance, unsigned short sub_num);
const char * konf_tree__get_line(const konf_tree_t * instance);
void konf_tree__set_depth(konf_tree_t * instance, int depth);
int konf_tree__get_depth(const konf_tree_t * instance);

#endif				/* _konf_tree_h */
/** @} clish_conf */
