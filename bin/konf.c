/*
 * konf.c
 *
 * The client to communicate to konfd configuration daemon.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#if WITH_INTERNAL_GETOPT
#include "libc/getopt.h"
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif

#include "konf/net.h"
#include "konf/query.h"
#include "konf/buf.h"
#include "lub/string.h"

#ifndef VERSION
#define VERSION 1.2.2
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", v)

static void help(int status, const char *argv0);

static const char *escape_chars = "\"\\'";

/*--------------------------------------------------------- */
int main(int argc, char **argv)
{
	int res = -1;
	konf_client_t *client = NULL;
	konf_buf_t *buf = NULL;
	char *line = NULL;
	char *str = NULL;
	const char *socket_path = KONFD_SOCKET_PATH;
	int i = 0;

	/* Signal vars */
	struct sigaction sigpipe_act;
	sigset_t sigpipe_set;

	static const char *shortopts = "hvs:";
#ifdef HAVE_GETOPT_LONG
	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{"socket",	1, NULL, 's'},
		{NULL,		0, NULL, 0}
	};
#endif

	/* Ignore SIGPIPE */
	sigemptyset(&sigpipe_set);
	sigaddset(&sigpipe_set, SIGPIPE);
	sigpipe_act.sa_flags = 0;
	sigpipe_act.sa_mask = sigpipe_set;
	sigpipe_act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigpipe_act, NULL);

	/* Parse command line options */
	while(1) {
		int opt;
#ifdef HAVE_GETOPT_LONG
		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
#else
		opt = getopt(argc, argv, shortopts);
#endif
		if (-1 == opt)
			break;
		switch (opt) {
		case 's':
			socket_path = optarg;
			break;
		case 'h':
			help(0, argv[0]);
			exit(0);
			break;
		case 'v':
			version(VERSION);
			exit(0);
			break;
		default:
			help(-1, argv[0]);
			exit(-1);
			break;
		}
	}

	/* Get request line from the args */
	for (i = optind; i < argc; i++) {
		char *space = NULL;
		if (NULL != line)
			lub_string_cat(&line, " ");
		space = strchr(argv[i], ' ');
		if (space)
			lub_string_cat(&line, "\"");
		str = lub_string_encode(argv[i], escape_chars);
		lub_string_cat(&line, str);
		lub_string_free(str);
		if (space)
			lub_string_cat(&line, "\"");
	}
	if (!line) {
		help(-1, argv[0]);
		goto err;
	}
#ifdef DEBUG
	fprintf(stderr, "REQUEST: %s\n", line);
#endif

	if (!(client = konf_client_new(socket_path))) {
		fprintf(stderr, "Error: Can't create internal data structures.\n");
		goto err;
	}

	if (konf_client_connect(client) < 0) {
		fprintf(stderr, "Error: Can't connect to %s socket.\n", socket_path);
		goto err;
	}

	if (konf_client_send(client, line) < 0) {
		fprintf(stderr, "Error: Can't send request to %s socket.\n", socket_path);
		goto err;
	}

	if (konf_client_recv_answer(client, &buf) < 0) {
		fprintf(stderr, "Error: The error code from the konfd daemon.\n");
		goto err;
	}

	if (buf) {
		konf_buf_lseek(buf, 0);
		while ((str = konf_buf_preparse(buf))) {
			if (strlen(str) == 0) {
				lub_string_free(str);
				break;
			}
			fprintf(stdout, "%s\n", str);
			lub_string_free(str);
		}
		konf_buf_delete(buf);
	}

	res = 0;
err:
	lub_string_free(line);
	konf_client_free(client);

	return res;
}

/*--------------------------------------------------------- */
/* Print help message */
static void help(int status, const char *argv0)
{
	const char *name = NULL;

	if (!argv0)
		return;

	/* Find the basename */
	name = strrchr(argv0, '/');
	if (name)
		name++;
	else
		name = argv0;

	if (status != 0) {
		fprintf(stderr, "Try `%s -h' for more information.\n",
			name);
	} else {
		printf("Usage: %s [options] -- <command for konfd daemon>\n", name);
		printf("Utility for communication to the konfd "
			"configuration daemon.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint utility version.\n");
		printf("\t-h, --help\tPrint this help.\n");
		printf("\t-s <path>, --socket=<path>\tSpecify listen socket "
			"of the konfd daemon.\n");
	}
}
