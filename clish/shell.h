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
#include "clish/plugin.h"
#include "konf/net.h"

#define CLISH_LOCK_PATH "/tmp/clish.lock"
#define CLISH_LOCK_WAIT 20

#define CLISH_XML_ERROR_STR "Error parsing XML: "
#define CLISH_XML_ERROR_ATTR(attr) CLISH_XML_ERROR_STR"The \""attr"\" attribute is required.\n"

typedef struct clish_shell_s clish_shell_t;
typedef struct clish_context_s clish_context_t;

/* Context functions */
_BEGIN_C_DECL
clish_context_t *clish_context_new(clish_shell_t *shell);
int clish_context_init(clish_context_t *instance, clish_shell_t *shell);
void clish_context_free(clish_context_t *instance);
clish_shell_t *clish_context__get_shell(const void *instance);
void clish_context__set_cmd(void *instance, const clish_command_t *cmd);
const clish_command_t *clish_context__get_cmd(const void *instance);
void clish_context__set_pargv(void *instance, clish_pargv_t *pargv);
clish_pargv_t *clish_context__get_pargv(const void *instance);
void clish_context__set_action(void *instance, const clish_action_t *action);
const clish_action_t *clish_context__get_action(const void *instance);
_END_C_DECL

/* Shell */
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

/*-----------------
 * meta functions
 *----------------- */

clish_shell_t *clish_shell_new(FILE * istream, FILE * ostream,
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
int clish_shell_exec_action(clish_context_t *context, char **out);
int clish_shell_execute(clish_context_t *context, char **out);
int clish_shell_forceline(clish_shell_t *instance, const char *line, char ** out);
int clish_shell_readline(clish_shell_t *instance, char ** out);
void clish_shell_dump(clish_shell_t * instance);
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
unsigned int clish_shell__get_depth(const clish_shell_t * instance);
void clish_shell__set_depth(clish_shell_t *instance, unsigned int depth);
const char *clish_shell__get_viewid(const clish_shell_t * instance);
const char *clish_shell__get_overview(const clish_shell_t * instance);
tinyrl_t *clish_shell__get_tinyrl(const clish_shell_t * instance);
void clish_shell__set_pwd(clish_shell_t *instance, const char * line,
	clish_view_t * view, char * viewid, clish_context_t *context);
char *clish_shell__get_pwd_line(const clish_shell_t * instance,
	 unsigned int index);
char *clish_shell__get_pwd_full(const clish_shell_t * instance,
	unsigned int depth);
clish_view_t *clish_shell__get_pwd_view(const clish_shell_t * instance,
	unsigned int index);
konf_client_t *clish_shell__get_client(const clish_shell_t * instance);
FILE *clish_shell__get_istream(const clish_shell_t * instance);
FILE *clish_shell__get_ostream(const clish_shell_t * instance);
void clish_shell__set_lockfile(clish_shell_t * instance, const char * path);
char * clish_shell__get_lockfile(clish_shell_t * instance);
int clish_shell__set_socket(clish_shell_t * instance, const char * path);
int clish_shell_load_scheme(clish_shell_t * instance, const char * xml_path);
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

/* Log functions */
void clish_shell__set_log(clish_shell_t *instance, bool_t log);
bool_t clish_shell__get_log(const clish_shell_t *instance);
void clish_shell__set_facility(clish_shell_t *instance, int facility);
int clish_shell__get_facility(clish_shell_t *instance);

int clish_shell_wdog(clish_shell_t *instance);
void clish_shell__set_wdog_timeout(clish_shell_t *instance,
	unsigned int timeout);
unsigned int clish_shell__get_wdog_timeout(const clish_shell_t *instance);
int clish_shell__save_history(const clish_shell_t *instance, const char *fname);
int clish_shell__restore_history(clish_shell_t *instance, const char *fname);
void clish_shell__stifle_history(clish_shell_t *instance, unsigned int stifle);
struct passwd *clish_shell__get_user(clish_shell_t *instance);
void clish_shell__set_dryrun(clish_shell_t *instance, bool_t dryrun);
bool_t clish_shell__get_dryrun(const clish_shell_t *instance);

/* Plugin functions */
clish_plugin_t * clish_shell_find_plugin(clish_shell_t *instance,
	const char *name);
clish_plugin_t * clish_shell_find_create_plugin(clish_shell_t *instance,
	const char *name);
int clish_shell_load_plugins(clish_shell_t *instance);
int clish_shell_link_plugins(clish_shell_t *instance);

/* Unresolved symbols functions */
clish_sym_t *clish_shell_find_sym(clish_shell_t *instance,
	const char *name, int type);
clish_sym_t *clish_shell_add_sym(clish_shell_t *instance,
	void *func, const char *name, int type);
clish_sym_t *clish_shell_add_unresolved_sym(clish_shell_t *instance,
	const char *name, int type);
clish_sym_t *clish_shell_get_hook(const clish_shell_t *instance, int type);

/* Hook wrappers */
void *clish_shell_check_hook(const clish_context_t *clish_context, int type);
CLISH_HOOK_CONFIG(clish_shell_exec_config);
CLISH_HOOK_LOG(clish_shell_exec_log);

/* User data functions */
void *clish_shell__get_udata(const clish_shell_t *instance, const char *name);
void *clish_shell__del_udata(clish_shell_t *instance, const char *name);
int clish_shell__set_udata(clish_shell_t *instance,
	const char *name, void *data);

/* Access functions */
int clish_shell_prepare(clish_shell_t *instance);

_END_C_DECL

#endif				/* _clish_shell_h */
/** @} clish_shell */
