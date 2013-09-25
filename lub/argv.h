/*
 * argv.h
 */
/**
\ingroup lub
\defgroup lub_argv argv
@{

\brief This utility provides a simple means of manipulating a vector of 
command textual words.

A word is either separated by whitespace, or if quotes are used a word is
defined by the scope of the quotes.

e.g.
\verbatim
one two "this is the third word" four
\endverbatim
contains four "words" the third of which is a string.

*/
#ifndef _lub_argv_h
#define _lub_argv_h

#include <stddef.h>

#include "c_decl.h"
#include "types.h"

_BEGIN_C_DECL
/**
 * This type is used to reference an instance of an argument vector
 */
typedef struct lub_argv_s lub_argv_t;

/*=====================================
 * ARGV INTERFACE
 *===================================== */
/**
 *  This operation is used to construct an instance of this class. The client
 * species a string and an offset within that string, from which to start
 * collecting "words" to place into the vector instance.
 *
 * \pre
 * - none
 *
 * \return
 * - A instance of an argument vector, which represents the words contained in
 * the provided string.
 * - NULL if there is insuffcient resource
 *
 * \post
 * - The client becomes resposible for releasing the instance when they are
 *   finished with it, by calling lub_argv_delete()
 */
lub_argv_t *lub_argv_new(
	/**
         * The string to analyse
         */
	const char *line,
	/**
         * The offset in the string to start from
         */
	size_t offset);

void lub_argv_delete(lub_argv_t * instance);
unsigned lub_argv__get_count(const lub_argv_t * instance);
const char *lub_argv__get_arg(const lub_argv_t * instance, unsigned index);
size_t lub_argv__get_offset(const lub_argv_t * instance, unsigned index);
bool_t lub_argv__get_quoted(const lub_argv_t * instance, unsigned index);
void lub_argv__set_arg(lub_argv_t * instance, unsigned index, const char *arg);
char **lub_argv__get_argv(const lub_argv_t * instance, const char *argv0);
void lub_argv__free_argv(char **argv);
char *lub_argv__get_line(const lub_argv_t * instance);
void lub_argv_add(lub_argv_t * instance, const char *text);

_END_C_DECL
#endif				/* _lub_argv_h */
/** @} lub_argv */
