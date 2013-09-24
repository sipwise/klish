#ifndef _lub_list_h
#define _lub_list_h

#include <stddef.h>
#include "lub/c_decl.h"

/****************************************************************
 * TYPE DEFINITIONS
 **************************************************************** */

typedef struct lub_list_node_s lub_list_node_t;

/**
 * This type defines a callback function which will compare two nodes
 * with each other
 *
 * \param clientnode 	the client node to compare
 * \param clientkey 	the key to compare with a node
 *
 * \return
 *     <0 if clientnode  < clientkey;
 *      0 if clientnode == clientkey;
 *     >0 if clientnode  > clientkey
 */
typedef int lub_list_compare_fn(const void *first, const void *second);

/**
 * This type represents a list instance
 */
typedef struct lub_list_s lub_list_t;

/**
 * This is used to perform iterations of a list
 */
typedef struct lub_list_node_s lub_list_iterator_t;

_BEGIN_C_DECL
/****************************************************************
 * LIST OPERATIONS
 **************************************************************** */
/**
 * This operation initialises an instance of a list.
 */
lub_list_t *lub_list_new(lub_list_compare_fn compareFn);
lub_list_node_t *lub_list_node_new(void *data);
void lub_list_free(lub_list_t *list);
void lub_list_node_free(lub_list_node_t *node);
lub_list_node_t *lub_list__get_head(lub_list_t *list);
lub_list_node_t *lub_list__get_tail(lub_list_t *list);
lub_list_node_t *lub_list_node__get_prev(lub_list_node_t *node);
lub_list_node_t *lub_list_node__get_next(lub_list_node_t *node);
void *lub_list_node__get_data(lub_list_node_t *node);
lub_list_node_t *lub_list_iterator_init(lub_list_t *list);
lub_list_node_t *lub_list_iterator_next(lub_list_node_t *node);
lub_list_node_t *lub_list_iterator_prev(lub_list_node_t *node);
lub_list_node_t *lub_list_add(lub_list_t *list, void *data);
void lub_list_del(lub_list_t *list, lub_list_node_t *node);
void lub_list_node_copy(lub_list_node_t *dst, lub_list_node_t *src);

_END_C_DECL
#endif				/* _lub_list_h */

