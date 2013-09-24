/*
 * ptype.h
 */
  /**
\ingroup clish
\defgroup clish_ptype ptype
@{

\brief This class represents a parameter type.

Types are a syntatical template which parameters reference.

*/

#ifndef _clish_ptype_h
#define _clish_ptype_h

typedef struct clish_ptype_s clish_ptype_t;

#include "lub/types.h"
#include "lub/bintree.h"
#include "lub/argv.h"

#include <stddef.h>

/*=====================================
 * PTYPE INTERFACE
 *===================================== */
/*-----------------
 * public types
 *----------------- */
/**
 * The means by which the pattern is interpreted and 
 * validated.
 */
typedef enum {
    /**
     * [default] - A POSIX regular expression.
    */
	CLISH_PTYPE_REGEXP,
    /**
     * A numeric definition "min..max" signed and unsigned versions
     */
	CLISH_PTYPE_INTEGER,
	CLISH_PTYPE_UNSIGNEDINTEGER,
    /**
     * A list of possible values. 
     * The syntax of the string is of the form: 
     *  "valueOne(ONE) valueTwo(TWO) valueThree(THREE)"
     * where the text before the parethesis defines the syntax 
     * that the user must use, and the value within the parenthesis 
     * is the result expanded as a parameter value. 
     */
	CLISH_PTYPE_SELECT
} clish_ptype_method_e;
/**
 * This defines the pre processing which is to be
 * performed before a string is validated.
 */
typedef enum {
    /**
     * [default] - do nothing
     */
	CLISH_PTYPE_NONE,
    /**
     * before validation convert to uppercase.
     */
	CLISH_PTYPE_TOUPPER,
    /**
     * before validation convert to lowercase.
     */
	CLISH_PTYPE_TOLOWER
} clish_ptype_preprocess_e;

/*-----------------
 * meta functions
 *----------------- */
int clish_ptype_bt_compare(const void *clientnode, const void *clientkey);
void clish_ptype_bt_getkey(const void *clientnode, lub_bintree_key_t * key);
size_t clish_ptype_bt_offset(void);
const char *clish_ptype_method__get_name(clish_ptype_method_e method);
clish_ptype_method_e clish_ptype_method_resolve(const char *method_name);
const char *clish_ptype_preprocess__get_name(clish_ptype_preprocess_e
	preprocess);
clish_ptype_preprocess_e clish_ptype_preprocess_resolve(const char
	*preprocess_name);
clish_ptype_t *clish_ptype_new(const char *name, const char *text,
	const char *pattern, clish_ptype_method_e method,
	clish_ptype_preprocess_e preprocess);
/*-----------------
 * methods
 *----------------- */
void clish_ptype_delete(clish_ptype_t * instance);
/**
 * This is the validation method for the specified type.
 * \return
 * - NULL if the validation is negative.
 * - A pointer to a string containing the validated text. NB. this
 *   may not be identical to that passed in. e.g. it may have been
 *   a case-modified "select" or a preprocessed value.
 */
char *clish_ptype_validate(const clish_ptype_t * instance, const char *text);
/**
 * This is the translation method for the specified type. The text is
 * first validated then translated into the form which should be used
 * for variable substitutions in ACTION or VIEW_ID fields.
 * \return
 * - NULL if the validation is negative.
 * - A pointer to a string containing the translated text. NB. this
 *   may not be identical to that passed in. e.g. it may have been
 *   a translated "select" value.
 */
char *clish_ptype_translate(const clish_ptype_t * instance, const char *text);
/**
 * This is used to perform parameter auto-completion
 */
void clish_ptype_word_generator(clish_ptype_t * instance,
	lub_argv_t *matches, const char *text);
void clish_ptype_dump(clish_ptype_t * instance);
/*-----------------
 * attributes
 *----------------- */
const char *clish_ptype__get_name(const clish_ptype_t * instance);
const char *clish_ptype__get_text(const clish_ptype_t * instance);
const char *clish_ptype__get_range(const clish_ptype_t * instance);
void clish_ptype__set_preprocess(clish_ptype_t * instance,
	clish_ptype_preprocess_e preprocess);
void clish_ptype__set_pattern(clish_ptype_t * instance,
	const char *pattern, clish_ptype_method_e method);
void clish_ptype__set_text(clish_ptype_t * instance, const char *text);
#endif	/* _clish_ptype_h */
/** @} clish_ptype */
