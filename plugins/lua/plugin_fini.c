#include <stdlib.h>

#include "private.h"

CLISH_PLUGIN_FINI(clish_plugin_lua_fini)
{
	lua_State *L = clish_shell__del_udata(clish_shell, LUA_UDATA);

	if (L)
		lua_close(L);

	return (0);
}
