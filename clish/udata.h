/*
 * udata.h
 */
 /**
\ingroup clish
\defgroup clish_udata udata
@{

\brief This class represents the top level container for CLI user data.
*/
#ifndef _clish_udata_h
#define _clish_udata_h

typedef struct clish_udata_s clish_udata_t;

/*=================================
 * USERDATA INTERFACE
 *================================= */
int clish_udata_compare(const void *first, const void *second);
clish_udata_t *clish_udata_new(const char *name, void *data);
void *clish_udata_free(clish_udata_t *instance);
void *clish_udata__get_data(const clish_udata_t *instance);
int clish_udata__set_data(clish_udata_t *instance, void *data);
char *clish_udata__get_name(const clish_udata_t *instance);

#endif				/* _clish_udata_h */
/** @} clish_udata */
