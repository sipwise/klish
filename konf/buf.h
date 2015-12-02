/*
 * buf.h
 */
 /**
\ingroup clish
\defgroup clish_conf config
@{

\brief This class is a config in memory container.

Use it to implement config in memory.

*/
#ifndef _konf_buf_h
#define _konf_buf_h

#include <stdio.h>

#include <lub/bintree.h>

typedef struct konf_buf_s konf_buf_t;

/*=====================================
 * CONF INTERFACE
 *===================================== */
/*-----------------
 * meta functions
 *----------------- */
konf_buf_t *konf_buf_new(int fd);
int konf_buf_bt_compare(const void *clientnode, const void *clientkey);
void konf_buf_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t konf_buf_bt_offset(void);
/*-----------------
 * methods
 *----------------- */
void konf_buf_delete(konf_buf_t *instance);
int konf_buf_read(konf_buf_t *instance);
int konf_buf_add(konf_buf_t *instance, void *str, size_t len);
char * konf_buf_string(char *instance, int len);
char * konf_buf_parse(konf_buf_t *instance);
char * konf_buf_preparse(konf_buf_t *instance);
int konf_buf_lseek(konf_buf_t *instance, int newpos);
int konf_buf__get_fd(const konf_buf_t *instance);
int konf_buf__get_len(const konf_buf_t *instance);
char * konf_buf__dup_line(const konf_buf_t *instance);
char * konf_buf__get_buf(const konf_buf_t *instance);
void * konf_buf__get_data(const konf_buf_t *instance);
void konf_buf__set_data(konf_buf_t *instance, void *data);

int konf_buftree_read(lub_bintree_t *instance, int fd);
char * konf_buftree_parse(lub_bintree_t *instance, int fd);
void konf_buftree_remove(lub_bintree_t *instance, int fd);
konf_buf_t *konf_buftree_find(lub_bintree_t *instance, int fd);

#endif				/* _konf_buf_h */
/** @} clish_conf */
