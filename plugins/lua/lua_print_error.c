#include "private.h"

void
l_print_error(lua_State *L, const char *func, const char *when, int res)
{
	switch (res) {
		case LUA_ERRRUN:
			printf("%s (%s):\n%s\n", func, when,
					lua_tostring(L, -1));
			break;
		case LUA_ERRSYNTAX:
			printf("%s (%s):\n%s\n", func, when,
					lua_tostring(L, -1));
			break;
		case LUA_ERRMEM:
			printf("%s (%s):\nmem alloc error\n", func, when);
			break;
		case LUA_ERRERR:
			printf("%s (%s):\nerror handler error\n", func, when);
			break;
		case LUA_ERRFILE:
			printf("%s (%s):\ncouldn't open file\n", func, when);
			break;
		default:
			printf("%s (%s):\nunknown error\n", func, when);
			break;
	}
}
