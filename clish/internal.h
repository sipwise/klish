/*
 * internal.h
 */
#ifndef _clish_internal_h
#define _clish_internal_h

#include "lub/c_decl.h"
#include "clish/shell.h"

_BEGIN_C_DECL

/* storage */
extern struct termios clish_default_tty_termios;

/* Standard clish callback functions */
extern clish_shell_access_fn_t clish_access_callback;
extern clish_shell_script_fn_t clish_script_callback;
extern clish_shell_script_fn_t clish_dryrun_callback;
extern clish_shell_config_fn_t clish_config_callback;
extern clish_shell_log_fn_t clish_log_callback;

_END_C_DECL

#endif /* _clish_internal_h */
