 /**
\ingroup tinyrl
\defgroup tinyrl_history history
@{

\brief This class handles the maintenance of a historical list of command lines.

*/
#ifndef _tinyrl_history_h
#define _tinyrl_history_h

#include "lub/c_decl.h"
#include "lub/types.h"

_BEGIN_C_DECL
/**************************************
 * tinyrl_history_entry class interface
 ************************************** */
typedef struct _tinyrl_history_entry tinyrl_history_entry_t;

extern const char *tinyrl_history_entry__get_line(const tinyrl_history_entry_t *
						  instance);
extern unsigned tinyrl_history_entry__get_index(const tinyrl_history_entry_t *
						instance);

/**************************************
 * tinyrl_history class interface
 ************************************** */
typedef struct _tinyrl_history tinyrl_history_t;

/**
 * This type is used for the iteration of history entries
 */
typedef struct _tinyrl_history_iterator tinyrl_history_iterator_t;
/**
 * CLIENTS MUST NOT USE THESE FIELDS DIRECTLY
 */
struct _tinyrl_history_iterator {
	const tinyrl_history_t *history;
	unsigned offset;
};

extern tinyrl_history_t *tinyrl_history_new(unsigned stifle);

extern void tinyrl_history_delete(tinyrl_history_t * instance);

extern void tinyrl_history_add(tinyrl_history_t * instance, const char *line);

extern tinyrl_history_entry_t *tinyrl_history_getfirst(const tinyrl_history_t *
						       instance,
						       tinyrl_history_iterator_t
						       * iter);
extern tinyrl_history_entry_t *tinyrl_history_getlast(const tinyrl_history_t *
						      instance,
						      tinyrl_history_iterator_t
						      * iter);

extern tinyrl_history_entry_t *tinyrl_history_getnext(tinyrl_history_iterator_t
						      * iter);

extern tinyrl_history_entry_t
    *tinyrl_history_getprevious(tinyrl_history_iterator_t * iter);

/*
HISTORY LIST MANAGEMENT 
*/
extern tinyrl_history_entry_t *tinyrl_history_remove(tinyrl_history_t *
						     instance, unsigned offset);
extern void tinyrl_history_clear(tinyrl_history_t * instance);
extern void tinyrl_history_stifle(tinyrl_history_t * instance, unsigned stifle);
extern unsigned tinyrl_history_unstifle(tinyrl_history_t * instance);
extern bool_t tinyrl_history_is_stifled(const tinyrl_history_t * instance);

extern int tinyrl_history_save(const tinyrl_history_t *instance, const char *fname);
extern int tinyrl_history_restore(tinyrl_history_t *instance, const char *fname);

    /*
       INFORMATION ABOUT THE HISTORY LIST 
     */
extern tinyrl_history_entry_t **tinyrl_history_list(const tinyrl_history_t *
						    instance);
extern tinyrl_history_entry_t *tinyrl_history_get(const tinyrl_history_t *
						  instance, unsigned offset);

/*
 * HISTORY EXPANSION 
 */
typedef enum {
	tinyrl_history_NO_EXPANSION,
	tinyrl_history_EXPANDED
} tinyrl_history_expand_t;

extern tinyrl_history_expand_t
tinyrl_history_expand(const tinyrl_history_t * instance,
		      const char *string, char **output);

_END_C_DECL
#endif				/* _tinyrl_history_h */
/** @} tinyrl_history */
