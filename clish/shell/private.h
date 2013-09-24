/*
 * shell.h - private interface to the shell class
 */
#include "lub/bintree.h"
#include "tinyrl/tinyrl.h"
#include "clish/shell.h"
#include "clish/pargv.h"
#include "clish/var.h"
#include "clish/action.h"

/*-------------------------------------
 * PRIVATE TYPES
 *------------------------------------- */

/*-------------------------------------------------------- */

/*
 * iterate around commands
 */
typedef struct {
	const char *last_cmd;
	clish_nspace_visibility_t field;
} clish_shell_iterator_t;

/* this is used to maintain a stack of file handles */
typedef struct clish_shell_file_s clish_shell_file_t;
struct clish_shell_file_s {
	clish_shell_file_t *next;
	FILE *file;
	char *fname;
	unsigned int line;
	bool_t stop_on_error; /* stop on error for file input  */
};

typedef struct {
	char *line;
	clish_view_t *view;
	lub_bintree_t viewid;
} clish_shell_pwd_t;

struct clish_shell_s {
	lub_bintree_t view_tree; /* Maintain a tree of views */
	lub_bintree_t ptype_tree; /* Maintain a tree of ptypes */
	lub_bintree_t var_tree; /* Maintain a tree of global variables */
	const clish_shell_hooks_t *client_hooks; /* Client callback hooks */
	void *client_cookie; /* Client callback cookie */
	clish_view_t *global; /* Reference to the global view. */
	clish_command_t *startup; /* This is the startup command */
	unsigned int idle_timeout; /* This is the idle timeout */
	clish_command_t *wdog; /* This is the watchdog command */
	unsigned int wdog_timeout; /* This is the watchdog timeout */
	bool_t wdog_active; /* If watchdog is active now */
	clish_shell_state_t state; /* The current state */
	char *overview; /* Overview text for this shell */
	tinyrl_t *tinyrl; /* Tiny readline instance */
	clish_shell_file_t *current_file; /* file currently in use for input */
	clish_shell_pwd_t **pwdv; /* Levels for the config file structure */
	unsigned int pwdc;
	int depth;
	konf_client_t *client;
	char *lockfile;
	char *default_shebang;
	char *fifo_name; /* The name of temporary fifo file. */
	bool_t interactive; /* Is shell interactive. */
	bool_t log; /* If command logging is enabled */
	struct passwd *user; /* Current user information */

	/* Static params for var expanding. The refactoring is needed. */
	clish_param_t *param_depth;
	clish_param_t *param_pwd;
};

/**
 * Initialise a command iterator structure
 */
void
clish_shell_iterator_init(clish_shell_iterator_t * iter,
	clish_nspace_visibility_t field);

/**
 * get the next command which is an extension of the specified line 
 */
const clish_command_t *clish_shell_find_next_completion(const clish_shell_t *
	instance, const char *line, clish_shell_iterator_t * iter);
/**
 * Pop the current file handle from the stack of file handles, shutting
 * the file down and freeing any associated memory. The next file handle
 * in the stack becomes associated with the input stream for this shell.
 *
 * \return
 * BOOL_TRUE - the current file handle has been replaced.
 * BOOL_FALSE - there is only one handle on the stack which cannot be replaced.
 */
int clish_shell_pop_file(clish_shell_t * instance);

clish_view_t *clish_shell_find_view(clish_shell_t * instance, const char *name);
void clish_shell_insert_view(clish_shell_t * instance, clish_view_t * view);
clish_pargv_status_t clish_shell_parse(clish_shell_t * instance,
	const char *line, const clish_command_t ** cmd, clish_pargv_t ** pargv);
clish_pargv_status_t clish_shell_parse_pargv(clish_pargv_t *pargv,
	const clish_command_t *cmd,
	void *context,
	clish_paramv_t *paramv,
	const lub_argv_t *argv,
	unsigned *idx, clish_pargv_t *last, unsigned need_index);
char *clish_shell_word_generator(clish_shell_t * instance,
	const char *line, unsigned offset, unsigned state);
const clish_command_t *clish_shell_resolve_command(const clish_shell_t *
	instance, const char *line);
const clish_command_t *clish_shell_resolve_prefix(const clish_shell_t *
	instance, const char *line);
void clish_shell_insert_ptype(clish_shell_t * instance, clish_ptype_t * ptype);
void clish_shell_tinyrl_history(clish_shell_t * instance, unsigned int *limit);
tinyrl_t *clish_shell_tinyrl_new(FILE * instream,
	FILE * outstream, unsigned stifle);
void clish_shell_tinyrl_delete(tinyrl_t * instance);
void clish_shell_param_generator(clish_shell_t * instance, lub_argv_t *matches,
	const clish_command_t * cmd, const char *line, unsigned offset);
char **clish_shell_tinyrl_completion(tinyrl_t * tinyrl,
	const char *line, unsigned start, unsigned end);
void clish_shell__expand_viewid(const char *viewid, lub_bintree_t *tree,
	clish_context_t *context);
void clish_shell__init_pwd(clish_shell_pwd_t *pwd);
void clish_shell__fini_pwd(clish_shell_pwd_t *pwd);
int clish_shell_timeout_fn(tinyrl_t *tinyrl);
int clish_shell_keypress_fn(tinyrl_t *tinyrl, int key);
