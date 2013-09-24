/*
 * shell.h
 */
 /**
\ingroup clish
\defgroup clish_shell shell
@{

\brief This class represents the top level container for a CLI session.

*/
#ifndef _clish_shell_h
#define _clish_shell_h

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include "lub/c_decl.h"
#include "lub/types.h"
#include "lub/argv.h"
#include "tinyrl/tinyrl.h"
#include "clish/view.h"
#include "clish/ptype.h"
#include "clish/var.h"
#include "konf/net.h"

#define CLISH_LOCK_PATH "/tmp/clish.lock"
#define CLISH_LOCK_WAIT 20

typedef struct clish_shell_s clish_shell_t;

/* This is used to hold context during callbacks */
struct clish_context_s {
	clish_shell_t *shell;
	const clish_command_t *cmd;
	clish_pargv_t *pargv;
};
typedef struct clish_context_s clish_context_t;

typedef enum {
	SHELL_STATE_OK = 0,
	SHELL_STATE_UNKNOWN = 1,
	SHELL_STATE_IO_ERROR = 2,
	SHELL_STATE_SCRIPT_ERROR = 3,/* Script execution error */
	SHELL_STATE_SYNTAX_ERROR = 4, /* Illegal line entered */
	SHELL_STATE_SYSTEM_ERROR = 5, /* Some internal system error */
	SHELL_STATE_INITIALISING = 6,
	SHELL_STATE_HELPING = 7,
	SHELL_STATE_EOF = 8, /* EOF of input stream */
	SHELL_STATE_CLOSING = 9
} clish_shell_state_t;

typedef enum {
	SHELL_VAR_NONE, /* Nothing to escape */
	SHELL_VAR_ACTION, /* Variable expanding for ACTION script */
	SHELL_VAR_REGEX /* Variable expanding for regex usage */
} clish_shell_var_t;

_BEGIN_C_DECL

/*=====================================
 * SHELL INTERFACE
 *===================================== */
 /**
  * A hook function used during the spawning of a new shell.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked just after the shell instance is created.
  *
  * This enables the client-specific initialisation of the spawned shell's
  * thread
  * e.g. to map the I/O streams, authenticate a user.
  *
  * N.B. It is possible for a client to have this invoked multiple times 
  * if the user is spawning new shells using a commmand which uses the 
  * "clish_spawn" builtin function. Hence the client should remember the 
  * shell which first calls this function, and only assign resource (e.g. 
  * setting up a script interpreter) for that call.
  *
  * \return
  * - BOOL_TRUE if everything is OK
  * - BOOL_FALSE if the shell should be immediately shut down.
  *
  */
typedef bool_t clish_shell_init_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					    const clish_shell_t * shell);

 /**
  * A hook function used during the shutting down of a spawned shell
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked just before the shell is destroyed.
  *
  * This enables the client-specific finalisation to occur.
  * e.g. releasing any resource held by the cookie,
  * shutting down telnet connections
  *
  * NB. This function may be called multiple times if a user is spawning
  * new commands (via the "clish_spawn" builtin command), hence should use
  * the reference to the root shell (remembered by the first call to clish_shell_init_fn_t callback)
  * to signal when the cleanup should occur.
  */
typedef void clish_shell_fini_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					  const clish_shell_t * shell);

 /**
  * A hook function used to indicate a command line has been executed and the 
  * shell is about to prompt for the next command.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called once an ACTION has been performed.
  * 
  * A client may use this as a periodic indicator of CLI activity, 
  * e.g. to manage session timeouts. In addition any required logging of 
  * commands may be performed.
  */
typedef void clish_shell_cmd_line_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      clish_context_t *context,
	/** 
         * The text of the command line entered
         */
					      const char *cmd_line);

/**
  * A hook function used to invoke the script associated with a command
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be invoked with the ACTION script which is to be performed.
  * 
  * The clish component will only pass down a call when a command has been
  * correctly input.
  *
  * The client may choose to implement invocation of the script in a number of
  * ways, which may include forking a sub-process or thread. It is important 
  * that the call doesn't return until the script has been fully evaluated.
  * 
  * \return 
  * - Retval (int)
  *
  * \post
  * - If the script executes successfully then any "view" tag associated with the
  *   command will be honored. i.e. the CLI will switch to the new view
  */
typedef int clish_shell_script_fn_t(
	clish_context_t *context,
	clish_action_t *action,
	const char *script,
	char **out);

/**
  * A hook function used to control config file write
  * 
  */
typedef bool_t clish_shell_config_fn_t(
	clish_context_t *context);

 /**
  * A hook function used to control access for the current user.
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called during the parsing of the XML files.
  * 
  * The clish component will only insert a command into a view if the access 
  * call is sucessfull.
  *
  * The client may choose to implement invocation of the script in a number of
  * ways, which may include forking a sub-process or thread. It is important 
  * that the call doesn't return until the script has been fully evaluated.
  *
  * \return
  * - BOOL_TRUE  - if the user of the current CLISH session is permitted access
  * - BOOL_FALSE - if the user of the current CLISH session is not permitted access
  *
  * \post
  * - If access is granted then the associated command will be inserted into the
  *   appropriate view.
  */
typedef bool_t clish_shell_access_fn_t(
	/** 
         * The shell instance which invoked this call
         */
					      const clish_shell_t * instance,
	/**
         * A textual string which describes a limitation for a command. This
         * string typically may be the name of a user group, of which the
         * current user must be a member to grant access to a command.  
         */
					      const char *access);
typedef int clish_shell_log_fn_t(
	clish_context_t *context,
	const char *line, int retcode);
 /**
  * A hook function used as a built in command callback
  * 
  * This will be invoked from the context of the spawned shell's thread
  * and will be called during the execution of a builting command.
  * 
  * A client may register any number of these callbacks in its 
  * clish_shell_builtin_cmds_t structure.
  *
  * \return
  * - Retval (int)
  *
  */
typedef int clish_shell_builtin_fn_t(
	clish_context_t *context,
	const lub_argv_t *argv,
	const char *script,
	char **out);

/** 
 * A client of libclish may provide some builtin commands which will be
 * interpreted by the framework, instead of the client's script engine.
 */
typedef struct {
	const char *name;		/**< The textual name to be used in 
                                         *    the 'builtin' XML attribute"
                                         */
	clish_shell_builtin_fn_t *callback;
					/**< The function to be invoked */
} clish_shell_builtin_t;

/** 
 * A client of libclish will provide hooks for the control of the CLI within 
 * a particular system.
 * They will populate an instance of this structure and pass it into the 
 */
typedef struct {
	clish_shell_init_fn_t *init_fn;         /* Initialisation call */
	clish_shell_access_fn_t *access_fn;     /* Access control call */
	clish_shell_cmd_line_fn_t *cmd_line_fn; /* Command line logging call */
	clish_shell_script_fn_t *script_fn;     /* script evaluation call */
	clish_shell_fini_fn_t *fini_fn;         /* Finalisation call */
	clish_shell_config_fn_t *config_fn;     /* Config call */
	clish_shell_log_fn_t *log_fn;           /* Logging call */
	const clish_shell_builtin_t *cmd_list;  /* NULL terminated list */
} clish_shell_hooks_t;
/*-----------------
 * meta functions
 *----------------- */

clish_shell_t *clish_shell_new(const clish_shell_hooks_t * hooks,
	void *cookie,
	FILE * istream,
	FILE * ostream,
	bool_t stop_on_error);
/*-----------------
 * methods
 *----------------- */
/*
 * Called to invoke the startup command for this shell
 */
int clish_shell_startup(clish_shell_t * instance);
void clish_shell_delete(clish_shell_t * instance);
clish_view_t *clish_shell_find_create_view(clish_shell_t * instance,
	const char *name,
	const char *prompt);
clish_ptype_t *clish_shell_find_create_ptype(clish_shell_t * instance,
	const char *name,
	const char *text,
	const char *pattern,
	clish_ptype_method_e method,
	clish_ptype_preprocess_e preprocess);
clish_ptype_t *clish_shell_find_ptype(clish_shell_t *instance,
	const char *name);
int clish_shell_xml_read(clish_shell_t * instance, const char *filename);
void clish_shell_help(clish_shell_t * instance, const char *line);
int clish_shell_exec_action(clish_action_t *action,
	clish_context_t *context, char **out);
int clish_shell_execute(clish_context_t *context, char **out);
int clish_shell_forceline(clish_shell_t *instance, const char *line, char ** out);
int clish_shell_readline(clish_shell_t *instance, char ** out);
void clish_shell_dump(clish_shell_t * instance);
void clish_shell_close(clish_shell_t * instance);
/**
 * Push the specified file handle on to the stack of file handles
 * for this shell. The specified file will become the source of 
 * commands, until it is exhausted.
 *
 * \return
 * BOOL_TRUE - the file was successfully associated with the shell.
 * BOOL_FALSE - there was insufficient resource to associate this file.
 */
int clish_shell_push_file(clish_shell_t * instance, const char * fname,
	bool_t stop_on_error);
int clish_shell_push_fd(clish_shell_t * instance, FILE * file,
	bool_t stop_on_error);
void clish_shell_insert_var(clish_shell_t *instance, clish_var_t *var);
clish_var_t *clish_shell_find_var(clish_shell_t *instance, const char *name);
char *clish_shell_expand_var(const char *name, clish_context_t *context);
char *clish_shell_expand(const char *str, clish_shell_var_t vtype, clish_context_t *context);

/*-----------------
 * attributes
 *----------------- */
clish_view_t *clish_shell__get_view(const clish_shell_t * instance);
unsigned clish_shell__get_depth(const clish_shell_t * instance);
const char *clish_shell__get_viewid(const clish_shell_t * instance);
const char *clish_shell__get_overview(const clish_shell_t * instance);
tinyrl_t *clish_shell__get_tinyrl(const clish_shell_t * instance);
void *clish_shell__get_client_cookie(const clish_shell_t * instance);
void
clish_shell__set_pwd(clish_shell_t *instance, const char * line,
	clish_view_t * view, char * viewid, clish_context_t *context);
char *clish_shell__get_pwd_line(const clish_shell_t * instance,
	 unsigned int index);
char *clish_shell__get_pwd_full(const clish_shell_t * instance, unsigned depth);
clish_view_t *clish_shell__get_pwd_view(const clish_shell_t * instance,
	unsigned int index);
konf_client_t *clish_shell__get_client(const clish_shell_t * instance);
FILE *clish_shell__get_istream(const clish_shell_t * instance);
FILE *clish_shell__get_ostream(const clish_shell_t * instance);
void clish_shell__set_lockfile(clish_shell_t * instance, const char * path);
char * clish_shell__get_lockfile(clish_shell_t * instance);
int clish_shell__set_socket(clish_shell_t * instance, const char * path);
void clish_shell_load_scheme(clish_shell_t * instance, const char * xml_path);
int clish_shell_loop(clish_shell_t * instance);
clish_shell_state_t clish_shell__get_state(const clish_shell_t * instance);
void clish_shell__set_state(clish_shell_t * instance,
	clish_shell_state_t state);
void clish_shell__set_startup_view(clish_shell_t * instance, const char * viewname);
void clish_shell__set_startup_viewid(clish_shell_t * instance, const char * viewid);
void clish_shell__set_default_shebang(clish_shell_t * instance, const char * shebang);
const char * clish_shell__get_default_shebang(const clish_shell_t * instance);
const char * clish_shell__get_fifo(clish_shell_t * instance);
void clish_shell__set_interactive(clish_shell_t * instance, bool_t interactive);
bool_t clish_shell__get_interactive(const clish_shell_t * instance);
bool_t clish_shell__get_utf8(const clish_shell_t * instance);
void clish_shell__set_utf8(clish_shell_t * instance, bool_t utf8);
void clish_shell__set_timeout(clish_shell_t *instance, int timeout);
char *clish_shell__get_line(clish_context_t *context);
char *clish_shell__get_full_line(clish_context_t *context);
char *clish_shell__get_params(clish_context_t *context);
void clish_shell__set_log(clish_shell_t *instance, bool_t log);
bool_t clish_shell__get_log(const clish_shell_t *instance);
int clish_shell_wdog(clish_shell_t *instance);
void clish_shell__set_wdog_timeout(clish_shell_t *instance,
	unsigned int timeout);
unsigned int clish_shell__get_wdog_timeout(const clish_shell_t *instance);
int clish_shell__save_history(const clish_shell_t *instance, const char *fname);
int clish_shell__restore_history(clish_shell_t *instance, const char *fname);
void clish_shell__stifle_history(clish_shell_t *instance, unsigned int stifle);
struct passwd *clish_shell__get_user(clish_shell_t *instance);

_END_C_DECL

#endif				/* _clish_shell_h */
/** @} clish_shell */
