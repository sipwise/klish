#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <konf/buf.h>
#include <lub/string.h>

#include "private.h"

static int exec_action(lua_State *L, const char *script)
{
	int res = 0;

	if ((res = luaL_loadstring(L, script))) {
		l_print_error(L, __func__, "load", res);
	} else if ((res = lua_pcall(L, 0, 0, 0))) {
		l_print_error(L, __func__, "exec", res);
	}

	lua_gc(L, LUA_GCCOLLECT, 0);

	return res;
}

CLISH_PLUGIN_SYM(clish_plugin_lua_action)
{
	clish_shell_t *shell = clish_context__get_shell(clish_context);
	lua_State *L = clish_shell__get_udata(shell, LUA_UDATA);
	konf_buf_t *buf;
	pid_t childpid;
	int res = 0, fd[2];

	if (!script) /* Nothing to do */
		return (0);

	if (!out) /* Handle trivial case */
		return exec_action(L, script);

	if (pipe(fd) == -1) {
		perror("pipe");
		return -1;
	}

	if ((childpid = fork()) == -1) {
		perror("fork");
		return -1;
	}

	if (childpid == 0) { /* Child */
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		exit(exec_action(L, script));
	} else { /* Parent */
		close(fd[1]);
		buf = konf_buf_new(fd[0]);
		while(konf_buf_read(buf) > 0);
		*out = konf_buf__dup_line(buf);
		konf_buf_delete(buf);
		close(fd[0]);
	}

	return WEXITSTATUS(res);
}
