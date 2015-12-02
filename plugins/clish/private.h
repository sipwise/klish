/*
 * builtin private.h
 */

#include "clish/plugin.h"
#include "clish/shell.h"

/* Hooks */
CLISH_HOOK_ACCESS(clish_hook_access);
CLISH_HOOK_CONFIG(clish_hook_config);
CLISH_HOOK_LOG(clish_hook_log);

/* Navy, etc. syms */
CLISH_PLUGIN_SYM(clish_close);
CLISH_PLUGIN_SYM(clish_source);
CLISH_PLUGIN_SYM(clish_source_nostop);
CLISH_PLUGIN_SYM(clish_overview);
CLISH_PLUGIN_SYM(clish_history);
CLISH_PLUGIN_SYM(clish_nested_up);
CLISH_PLUGIN_SYM(clish_nop);
CLISH_PLUGIN_SYM(clish_wdog);
CLISH_PLUGIN_OSYM(clish_script);
CLISH_PLUGIN_SYM(clish_macros);
