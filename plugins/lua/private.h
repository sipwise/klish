#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <clish/shell.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define LUA_UDATA "lua_state"
#define LUA_SCRIPTS_DIR "scripts_dir"
#define LUA_PLUGIN_NAME "lua"

extern char *scripts_path;
void l_print_error(lua_State *, const char *, const char *, int);
int clish_plugin_init_lua(clish_shell_t *shell);

CLISH_PLUGIN_SYM(clish_plugin_lua_action);

#endif /* _PLUGIN_H_ */
