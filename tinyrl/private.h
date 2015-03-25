#include <termios.h>

#include "tinyrl/tinyrl.h"
#include "tinyrl/vt100.h"

/* define the class member data and virtual methods */
struct _tinyrl {
	const char *line;
	unsigned max_line_length;
	char *prompt;
	size_t prompt_size; /* strlen() */
	size_t prompt_len; /* Symbol positions */
	char *buffer;
	size_t buffer_size;
	bool_t done;
	bool_t completion_over;
	bool_t completion_error_over;
	unsigned point;
	unsigned end;
	tinyrl_completion_func_t *attempted_completion_function;
	tinyrl_timeout_fn_t *timeout_fn; /* timeout callback */
	tinyrl_keypress_fn_t *keypress_fn; /* keypress callback */
	int state;
#define RL_STATE_COMPLETING (0x00000001)
	char *kill_string;
#define NUM_HANDLERS 256
	tinyrl_key_func_t *handlers[NUM_HANDLERS];
	tinyrl_key_func_t *hotkey_fn;

	tinyrl_history_t *history;
	tinyrl_history_iterator_t hist_iter;
	tinyrl_vt100_t *term;
	void *context;		/* context supplied by caller
				 * to tinyrl_readline()
				 */
	char echo_char;
	bool_t echo_enabled;
	struct termios default_termios;
	bool_t isatty;
	char *last_buffer;	/* hold record of the previous
				buffer for redisplay purposes */
	unsigned int last_point; /* hold record of the previous
				cursor position for redisplay purposes */
	unsigned int last_line_size; /* The length of last_buffer */
	unsigned int last_width; /* Last terminal width. For resize */
	bool_t utf8;		/* Is the encoding UTF-8 */
};
