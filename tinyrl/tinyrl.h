 /**
\ingroup tinyrl
\defgroup tinyrl_class tinyrl
@{

\brief This class provides instances which are capable of handling user input
from a CLI in a "readline" like fashion.

*/
#ifndef _tinyrl_tinyrl_h
#define _tinyrl_tinyrl_h

#include <stdio.h>
#include "lub/types.h"
#include "lub/c_decl.h"
#include "tinyrl/history.h"

_BEGIN_C_DECL typedef struct _tinyrl tinyrl_t;
typedef enum {
    /**
     * no possible completions were found
     */
	TINYRL_NO_MATCH = 0,
    /**
     * the provided string was already an exact match
     */
	TINYRL_MATCH,
    /**
     * the provided string was ambiguous and produced
     * more than one possible completion
     */
	TINYRL_AMBIGUOUS,
    /**
     * the provided string was unambiguous and a 
     * completion was performed
     */
	TINYRL_COMPLETED_MATCH,
    /**
     * the provided string was ambiguous but a partial
     * completion was performed.
     */
	TINYRL_COMPLETED_AMBIGUOUS,
    /**
     * the provided string was an exact match for one
     * possible value but there are other exetensions
     * of the string available.
     */
	TINYRL_MATCH_WITH_EXTENSIONS
} tinyrl_match_e;

/* virtual methods */
typedef char *tinyrl_compentry_func_t(tinyrl_t * instance,
	const char *text, unsigned offset, unsigned state);
typedef int tinyrl_hook_func_t(tinyrl_t * instance);

typedef char **tinyrl_completion_func_t(tinyrl_t * instance,
	const char *text, unsigned start, unsigned end);

typedef int tinyrl_timeout_fn_t(tinyrl_t *instance);
typedef int tinyrl_keypress_fn_t(tinyrl_t *instance, int key);

/**
 * \return
 * - BOOL_TRUE if the action associated with the key has
 *   been performed successfully
 * - BOOL_FALSE if the action was not successful
 */
typedef bool_t tinyrl_key_func_t(tinyrl_t * instance, int key);

/* exported functions */
extern tinyrl_t *tinyrl_new(FILE * instream,
			    FILE * outstream,
			    unsigned stifle,
			    tinyrl_completion_func_t * complete_fn);

/*lint -esym(534,tinyrl_printf)  Ignoring return value of function */
extern int tinyrl_printf(const tinyrl_t * instance, const char *fmt, ...);

extern void tinyrl_delete(tinyrl_t * instance);

extern tinyrl_history_t *tinyrl__get_history(const tinyrl_t * instance);

extern const char *tinyrl__get_prompt(const tinyrl_t * instance);
extern void tinyrl__set_prompt(tinyrl_t *instance, const char *prompt);

extern void tinyrl_done(tinyrl_t * instance);

extern void tinyrl_completion_over(tinyrl_t * instance);

extern void tinyrl_completion_error_over(tinyrl_t * instance);

extern bool_t tinyrl_is_completion_error_over(const tinyrl_t * instance);

extern void *tinyrl__get_context(const tinyrl_t * instance);

/**
 * This operation returns the current line in use by the tinyrl instance
 * NB. the pointer will become invalid after any further operation on the 
 * instance.
 */
extern const char *tinyrl__get_line(const tinyrl_t * instance);
extern void tinyrl__set_istream(tinyrl_t * instance, FILE * istream);
extern bool_t tinyrl__get_isatty(const tinyrl_t * instance);
extern FILE *tinyrl__get_istream(const tinyrl_t * instance);
extern FILE *tinyrl__get_ostream(const tinyrl_t * instance);
extern bool_t tinyrl__get_utf8(const tinyrl_t * instance);
extern void tinyrl__set_utf8(tinyrl_t * instance, bool_t utf8);
extern void tinyrl__set_timeout(tinyrl_t *instance, int timeout);
extern void tinyrl__set_timeout_fn(tinyrl_t *instance,
	tinyrl_timeout_fn_t *fn);
extern void tinyrl__set_keypress_fn(tinyrl_t *instance,
	tinyrl_keypress_fn_t *fn);
extern void tinyrl__set_hotkey_fn(tinyrl_t *instance,
	tinyrl_key_func_t *fn);
extern char *tinyrl_readline(tinyrl_t *instance, void *context);
extern char *tinyrl_forceline(tinyrl_t *instance, 
	void *context, const char *line);
extern bool_t tinyrl_bind_key(tinyrl_t *instance, int key,
	tinyrl_key_func_t *fn);
extern void tinyrl_delete_matches(char **instance);
extern char **tinyrl_completion(tinyrl_t *instance,
	const char *line, unsigned start, unsigned end,
	tinyrl_compentry_func_t *generator);
extern void tinyrl_crlf(const tinyrl_t * instance);
extern void tinyrl_multi_crlf(const tinyrl_t * instance);
extern void tinyrl_ding(const tinyrl_t * instance);

extern void tinyrl_reset_line_state(tinyrl_t * instance);

extern bool_t tinyrl_insert_text(tinyrl_t * instance, const char *text);
extern void
tinyrl_delete_text(tinyrl_t * instance, unsigned start, unsigned end);
extern void tinyrl_redisplay(tinyrl_t * instance);

extern void
tinyrl_replace_line(tinyrl_t * instance, const char *text, int clear_undo);

/**
 * Complete the current word in the input buffer, displaying
 * a prompt to clarify any abiguity if necessary.
 *
 * \return
 * - the type of match performed.
 * \post
 * - If the current word is ambiguous then a list of 
 *   possible completions will be displayed.
 */
extern tinyrl_match_e tinyrl_complete(tinyrl_t * instance);

/**
 * Complete the current word in the input buffer, displaying
 * a prompt to clarify any abiguity or extra extensions if necessary.
 *
 * \return
 * - the type of match performed.
 * \post
 * - If the current word is ambiguous then a list of 
 *   possible completions will be displayed.
 * - If the current word is complete but there are extra
 *   completions which are an extension of that word then
 *   a list of these will be displayed.
 */
extern tinyrl_match_e tinyrl_complete_with_extensions(tinyrl_t * instance);

/**
 * Disable echoing of input characters when a line in input.
 * 
 */
extern void tinyrl_disable_echo(
	 /** 
          * The instance on which to operate
          */
				       tinyrl_t * instance,
	 /**
          * The character to display instead of a key press.
          *
          * If this has the special value '/0' then the insertion point will not 
          * be moved when keys are pressed.
          */
				       char echo_char);
/**
 * Enable key echoing for this instance. (This is the default behaviour)
 */
extern void tinyrl_enable_echo(
	/** 
         * The instance on which to operate
         */
				      tinyrl_t * instance);
/**
 * Indicate whether the current insertion point is quoting or not
 */
extern bool_t tinyrl_is_quoting(
	/** 
         * The instance on which to operate
         */
				       const tinyrl_t * instance);
/**
 * Indicate whether the current insertion is empty or not
 */
extern bool_t
	tinyrl_is_empty(
		/**
		 * The instance on which to operate
		 */
		const tinyrl_t *instance
	);
/**
 * Limit maximum line length
 */
extern void tinyrl_limit_line_length(
	/** 
         * The instance on which to operate
         */
					    tinyrl_t * instance,
	/** 
         * The length to limit to (0) is unlimited
         */
					    unsigned length);

extern unsigned tinyrl__get_width(const tinyrl_t *instance);
extern unsigned tinyrl__get_height(const tinyrl_t *instance);
extern int tinyrl__save_history(const tinyrl_t *instance, const char *fname);
extern int tinyrl__restore_history(tinyrl_t *instance, const char *fname);
extern void tinyrl__stifle_history(tinyrl_t *instance, unsigned int stifle);

_END_C_DECL
#endif				/* _tinyrl_tinyrl_h */
/** @} tinyrl_tinyrl */
