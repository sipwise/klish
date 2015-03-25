/*
 * sigexec.c
 *
 * Programm to execute processes with unblocked signals.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#if WITH_INTERNAL_GETOPT
#include "libc/getopt.h"
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif

#ifndef VERSION
#define VERSION 1.5.6
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", v)

static void help(int status, const char *argv0);

int main(int argc, char *argv[])
{
	char **child_argv;
	sigset_t sigs;

	static const char *shortopts = "+hv";
#ifdef HAVE_GETOPT_LONG
	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{NULL,		0, NULL, 0}
	};
#endif

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
		case 0:
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
			help(1, argv[0]);
			break;
		}
	}

	child_argv = &argv[optind];
	/* Check user command */
	if (! child_argv[0]) {
		fprintf(stderr, "Error: Nothing to execute.\n");
		return 1;
	}

	/* Unblock signals */
	sigemptyset(&sigs);
	sigprocmask(SIG_SETMASK, &sigs, NULL);

	/* Execute user command */
/*	fprintf(stderr, "%s %s %s\n", child_argv[0], child_argv[1], child_argv[2]); */
	if (execvp(child_argv[0], child_argv) < 0) {
		fprintf(stderr, "Error: Cannot execute %s: %s\n",
			child_argv[0], strerror(errno));
		return 1;
	}

	return 0;
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
		printf("Usage: %s [options] -- <command to execute>\n", name);
		printf("Utility to execute process with unblocked signals.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint utility version.\n");
		printf("\t-h, --help\tPrint this help.\n");
	}
}
