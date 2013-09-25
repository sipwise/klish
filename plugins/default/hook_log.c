/*
 * callback_log.c
 *
 * Callback hook to log users's commands
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <pwd.h>

#include "clish/shell.h"

#define SYSLOG_IDENT "klish"
#define SYSLOG_FACILITY LOG_LOCAL0

/*--------------------------------------------------------- */
CLISH_HOOK_LOG(clish_hook_log)
{
	clish_shell_t *this = clish_context__get_shell(clish_context);
	struct passwd *user = NULL;
	char *uname = "unknown";

	/* Initialization */
	if (!line) {
		openlog(SYSLOG_IDENT, LOG_PID, SYSLOG_FACILITY);
		return 0;
	}

	/* Log the given line */
	if ((user = clish_shell__get_user(this)))
		uname = user->pw_name;
	syslog(LOG_INFO, "(%s) %s : %d", uname, line, retcode);

	return 0;
}
