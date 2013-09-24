/*
 * clish_config_callback.c
 *
 *
 * Callback hook to execute config operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>
#include <string.h>

#include "internal.h"
#include "konf/net.h"
#include "konf/buf.h"
#include "konf/query.h"
#include "lub/string.h"

static int send_request(konf_client_t * client, char *command);

static unsigned short str2ushort(const char *str)
{
	unsigned short num = 0;

	if (str && (*str != '\0')) {
		long val = 0;
		char *endptr;

		val = strtol(str, &endptr, 0);
		if (endptr == str)
			num = 0;
		else if (val > 0xffff)
			num = 0xffff;
		else if (val < 0)
			num = 0;
		else
			num = (unsigned)val;
	}

	return num;
}

/*--------------------------------------------------------- */
bool_t clish_config_callback(clish_context_t *context)
{
	clish_shell_t *this = context->shell;
	const clish_command_t *cmd = context->cmd;
	clish_config_t *config;
	char *command = NULL;
	konf_client_t *client;
	konf_buf_t *buf = NULL;
	char *str = NULL;
	char *tstr;
	char tmp[PATH_MAX + 100];
	clish_config_op_t op;
	unsigned int num;
	const char *escape_chars = lub_string_esc_quoted;

	if (!this)
		return BOOL_TRUE;

	client = clish_shell__get_client(this);
	if (!client)
		return BOOL_TRUE;

	config = clish_command__get_config(cmd);
	op = clish_config__get_op(config);

	switch (op) {

	case CLISH_CONFIG_NONE:
		return BOOL_TRUE;

	case CLISH_CONFIG_SET:
		/* Add set operation */
		lub_string_cat(&command, "-s");

		/* Add entered line */
		tstr = clish_shell__get_line(context);
		str = lub_string_encode(tstr, escape_chars);
		lub_string_free(tstr);
		lub_string_cat(&command, " -l \"");
		lub_string_cat(&command, str);
		lub_string_cat(&command, "\"");
		lub_string_free(str);

		/* Add splitter */
		if (!clish_config__get_splitter(config))
			lub_string_cat(&command, " -i");

		/* Add unique */
		if (!clish_config__get_unique(config))
			lub_string_cat(&command, " -n");

		break;

	case CLISH_CONFIG_UNSET:
		/* Add unset operation */
		lub_string_cat(&command, "-u");
		break;

	case CLISH_CONFIG_DUMP:
		/* Add dump operation */
		lub_string_cat(&command, "-d");

		/* Add filename */
		str = clish_shell_expand(clish_config__get_file(config), SHELL_VAR_ACTION, context);
		if (str) {
			lub_string_cat(&command, " -f \"");
			if (str[0] != '\0')
				lub_string_cat(&command, str);
			else
				lub_string_cat(&command, "/tmp/running-config");
			lub_string_cat(&command, "\"");
			lub_string_free(str);
		}
		break;

	default:
		return BOOL_FALSE;
	};

	/* Add pattern */
	if ((CLISH_CONFIG_SET == op) || (CLISH_CONFIG_UNSET == op)) {
		tstr = clish_shell_expand(clish_config__get_pattern(config), SHELL_VAR_REGEX, context);
		if (!tstr) {
			lub_string_free(command);
			return BOOL_FALSE;
		}
		str = lub_string_encode(tstr, escape_chars);
		lub_string_free(tstr);
		lub_string_cat(&command, " -r \"");
		lub_string_cat(&command, str);
		lub_string_cat(&command, "\"");
		lub_string_free(str);
	}

	/* Add priority */
	if (clish_config__get_priority(config) != 0) {
		snprintf(tmp, sizeof(tmp) - 1, " -p 0x%x",
			clish_config__get_priority(config));
		tmp[sizeof(tmp) - 1] = '\0';
		lub_string_cat(&command, tmp);
	}

	/* Add sequence */
	if (clish_config__get_seq(config)) {
		str = clish_shell_expand(clish_config__get_seq(config), SHELL_VAR_ACTION, context);
		snprintf(tmp, sizeof(tmp) - 1, " -q %u", str2ushort(str));
		tmp[sizeof(tmp) - 1] = '\0';
		lub_string_cat(&command, tmp);
		lub_string_free(str);
	}

	/* Add pwd */
	if (clish_config__get_depth(config)) {
		str = clish_shell_expand(clish_config__get_depth(config), SHELL_VAR_ACTION, context);
		num = str2ushort(str);
		lub_string_free(str);
	} else {
		num = clish_command__get_depth(cmd);
	}
	str = clish_shell__get_pwd_full(this, num);
	if (str) {
		lub_string_cat(&command, " ");
		lub_string_cat(&command, str);
		lub_string_free(str);
	}

#ifdef DEBUG
	fprintf(stderr, "CONFIG request: %s\n", command);
#endif
	if (send_request(client, command) < 0) {
		fprintf(stderr, "Cannot write to the running-config.\n");
	}
	if (konf_client_recv_answer(client, &buf) < 0) {
		fprintf(stderr, "The error while request to the config daemon.\n");
	}
	lub_string_free(command);

	/* Postprocessing. Get data from daemon etc. */
	switch (op) {

	case CLISH_CONFIG_DUMP:
		if (buf) {
			konf_buf_lseek(buf, 0);
			while ((str = konf_buf_preparse(buf))) {
				if (strlen(str) == 0) {
					lub_string_free(str);
					break;
				}
				tinyrl_printf(clish_shell__get_tinyrl(this),
					"%s\n", str);
				lub_string_free(str);
			}
			konf_buf_delete(buf);
		}
		break;

	default:
		break;
	};

	return BOOL_TRUE;
}

/*--------------------------------------------------------- */

static int send_request(konf_client_t * client, char *command)
{
	if ((konf_client_connect(client) < 0))
		return -1;

	if (konf_client_send(client, command) < 0) {
		if (konf_client_reconnect(client) < 0)
			return -1;
		if (konf_client_send(client, command) < 0)
			return -1;
	}

	return 0;
}
