#include "lub/log.h"
#include <syslog.h>
#include "lub/string.h"

int lub_log_facility(const char *str, int *facility)
{
	if (!lub_string_nocasecmp(str, "local0"))
		*facility = LOG_LOCAL0;
	else if (!lub_string_nocasecmp(str, "local1"))
		*facility = LOG_LOCAL1;
	else if (!lub_string_nocasecmp(str, "local2"))
		*facility = LOG_LOCAL2;
	else if (!lub_string_nocasecmp(str, "local3"))
		*facility = LOG_LOCAL3;
	else if (!lub_string_nocasecmp(str, "local4"))
		*facility = LOG_LOCAL4;
	else if (!lub_string_nocasecmp(str, "local5"))
		*facility = LOG_LOCAL5;
	else if (!lub_string_nocasecmp(str, "local6"))
		*facility = LOG_LOCAL6;
	else if (!lub_string_nocasecmp(str, "local7"))
		*facility = LOG_LOCAL7;
	else if (!lub_string_nocasecmp(str, "auth"))
		*facility = LOG_AUTH;
#ifdef LOG_AUTHPRIV
	else if (!lub_string_nocasecmp(str, "authpriv"))
		*facility = LOG_AUTHPRIV;
#endif
	else if (!lub_string_nocasecmp(str, "cron"))
		*facility = LOG_CRON;
	else if (!lub_string_nocasecmp(str, "daemon"))
		*facility = LOG_DAEMON;
#ifdef LOG_FTP
	else if (!lub_string_nocasecmp(str, "ftp"))
		*facility = LOG_FTP;
#endif
	else if (!lub_string_nocasecmp(str, "kern"))
		*facility = LOG_KERN;
	else if (!lub_string_nocasecmp(str, "lpr"))
		*facility = LOG_LPR;
	else if (!lub_string_nocasecmp(str, "mail"))
		*facility = LOG_MAIL;
	else if (!lub_string_nocasecmp(str, "news"))
		*facility = LOG_NEWS;
	else if (!lub_string_nocasecmp(str, "syslog"))
		*facility = LOG_SYSLOG;
	else if (!lub_string_nocasecmp(str, "user"))
		*facility = LOG_USER;
	else if (!lub_string_nocasecmp(str, "uucp"))
		*facility = LOG_UUCP;
	else
		return -1;

	return 0;
}
