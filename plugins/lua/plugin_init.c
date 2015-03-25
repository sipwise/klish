#include "lub/ini.h"
#include "lub/string.h"

#include "private.h"

CLISH_PLUGIN_INIT(lua)
{
	lub_ini_t *ini;
	char *conf = clish_plugin__get_conf(plugin);

	if (conf) {
		ini = lub_ini_new();
		lub_ini_parse_str(ini, conf);
		scripts_path =
			lub_string_dup(lub_ini_find(ini, LUA_SCRIPTS_DIR));
		lub_ini_free(ini);
	}

	if(clish_plugin_init_lua(clish_shell))
		return (-1);

	clish_plugin_add_fini(plugin, clish_plugin_lua_fini);
	clish_plugin_add_sym(plugin, clish_plugin_lua_action, "lua");

	return 0;
}
