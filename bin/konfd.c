/*
 * konfd.c
 *
 * The konfd daemon to store user configuration commands.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <syslog.h>

#if WITH_INTERNAL_GETOPT
#include "libc/getopt.h"
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#include "konf/tree.h"
#include "konf/query.h"
#include "konf/buf.h"
#include "konf/net.h"
#include "lub/argv.h"
#include "lub/string.h"
#include "lub/log.h"

#ifndef VERSION
#define VERSION 1.2.2
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", v)

#define KONFD_PIDFILE "/var/run/konfd.pid"

/* UNIX socket path */
/* Don't use UNIX_PATH_MAX due to portability issues */
#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

/* OpenBSD has no MSG_NOSIGNAL flag */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* Global signal vars */
static volatile int sigterm = 0;
static void sighandler(int signo);

static void help(int status, const char *argv0);
static char * process_query(konf_buf_t *tbuf, konf_tree_t * conf, char *str);
int answer_send(int sock, const char *command);
static int dump_running_config(int sock, konf_tree_t *conf, konf_query_t *query);
int daemonize(int nochdir, int noclose);
struct options *opts_init(void);
void opts_free(struct options *opts);
static int opts_parse(int argc, char *argv[], struct options *opts);
static int create_listen_socket(const char *path,
	uid_t uid, gid_t gid, mode_t mode);

/* Command line options */
struct options {
	char *socket_path;
	char *ro_path;
	char *pidfile;
	char *chroot;
	int debug; /* Don't daemonize in debug mode */
	uid_t uid;
	gid_t gid;
	int log_facility;
};

/*--------------------------------------------------------- */
int main(int argc, char **argv)
{
	int retval = -1;
	int i;
	char *str;
	konf_tree_t *conf;
	lub_bintree_t bufs;
	konf_buf_t *tbuf;
	struct options *opts = NULL;
	int pidfd = -1;

	/* Network vars */
	int sock = -1;
	int ro_sock = -1;
	struct sockaddr_un raddr;
	fd_set active_fd_set, read_fd_set;

	/* Signal vars */
	struct sigaction sig_act, sigpipe_act;
	sigset_t sig_set, sigpipe_set;

	/* Parse command line options */
	opts = opts_init();
	if (opts_parse(argc, argv, opts))
		goto err;

	/* Initialize syslog */
	openlog(argv[0], LOG_CONS, opts->log_facility);
	syslog(LOG_ERR, "Start daemon.\n");

	/* Fork the daemon */
	if (!opts->debug) {
		/* Daemonize */
		if (daemonize(0, 0) < 0) {
			syslog(LOG_ERR, "Can't daemonize\n");
			goto err;
		}

		/* Write pidfile */
		if ((pidfd = open(opts->pidfile,
			O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			syslog(LOG_WARNING, "Can't open pidfile %s: %s",
				opts->pidfile, strerror(errno));
		} else {
			char str[20];
			snprintf(str, sizeof(str), "%u\n", getpid());
			str[sizeof(str) - 1] = '\0';
			if (write(pidfd, str, strlen(str)) < 0)
				syslog(LOG_WARNING, "Can't write to %s: %s",
					opts->pidfile, strerror(errno));
			close(pidfd);
		}
	}

	/* Create RW listen socket */
	if ((sock = create_listen_socket(opts->socket_path,
		opts->uid, opts->gid, 0660)) == -1) {
		goto err;
	}

	/* Create RO listen socket */
	if (opts->ro_path && (ro_sock = create_listen_socket(opts->ro_path,
		opts->uid, opts->gid, 0666)) == -1) {
		goto err;
	}

	/* Change GID */
	if (opts->gid != getgid()) {
		if (setgid(opts->gid)) {
			syslog(LOG_ERR, "Can't set GID to %u: %s",
				opts->gid, strerror(errno));
			goto err;
		}
	}

#ifdef HAVE_CHROOT
	/* Chroot */
	if (opts->chroot) {
		if (chroot(opts->chroot) < 0) {
			syslog(LOG_ERR, "Can't chroot to %s: %s",
				opts->chroot, strerror(errno));
			goto err;
		}
	}
#endif

	/* Change UID */
	if (opts->uid != getuid()) {
		if (setuid(opts->uid)) {
			syslog(LOG_ERR, "Can't set UID to %u: %s",
				opts->uid, strerror(errno));
			goto err;
		}
	}

	/* Create configuration tree */
	conf = konf_tree_new("", 0);

	/* Initialize the tree of buffers */
	lub_bintree_init(&bufs,
		konf_buf_bt_offset(),
		konf_buf_bt_compare, konf_buf_bt_getkey);

	/* Set signal handler */
	sigemptyset(&sig_set);
	sigaddset(&sig_set, SIGTERM);
	sigaddset(&sig_set, SIGINT);
	sigaddset(&sig_set, SIGQUIT);

	sig_act.sa_flags = 0;
	sig_act.sa_mask = sig_set;
	sig_act.sa_handler = &sighandler;
	sigaction(SIGTERM, &sig_act, NULL);
	sigaction(SIGINT, &sig_act, NULL);
	sigaction(SIGQUIT, &sig_act, NULL);

	/* Ignore SIGPIPE */
	sigemptyset(&sigpipe_set);
	sigaddset(&sigpipe_set, SIGPIPE);
	sigpipe_act.sa_flags = 0;
	sigpipe_act.sa_mask = sigpipe_set;
	sigpipe_act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigpipe_act, NULL);

	/* Initialize the set of active sockets. */
	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);
	if (ro_sock >= 0)
		FD_SET(ro_sock, &active_fd_set);

	/* Main loop */
	while (!sigterm) {
		int num;

		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		num = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
		if (num < 0) {
			if (EINTR == errno)
				continue;
			break;
		}
		if (0 == num)
			continue;

		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (!FD_ISSET(i, &read_fd_set))
				continue;
			/* Connection request on listen socket. */
			if ((i == sock) || (i == ro_sock)) {
				int new;
				socklen_t size = sizeof(raddr);
				new = accept(i,
					(struct sockaddr *)&raddr, &size);
				if (new < 0) {
					continue;
				}
#ifdef DEBUG
				fprintf(stderr, "------------------------------\n");
				fprintf(stderr, "Connection established %u\n", new);
#endif
				konf_buftree_remove(&bufs, new);
				tbuf = konf_buf_new(new);
				/* Insert it into the binary tree */
				lub_bintree_insert(&bufs, tbuf);
				/* In a case of RW socket we use buf's data pointer
				  to indicate RW or RO socket. NULL=RO, not-NULL=RW */
				if (i == sock)
					konf_buf__set_data(tbuf, (void *)1);
				FD_SET(new, &active_fd_set);
			} else {
				int nbytes;

				tbuf = konf_buftree_find(&bufs, i);
				/* Data arriving on an already-connected socket. */
				if ((nbytes = konf_buf_read(tbuf)) <= 0) {
					close(i);
					FD_CLR(i, &active_fd_set);
					konf_buftree_remove(&bufs, i);
#ifdef DEBUG
					fprintf(stderr, "Connection closed %u\n", i);
#endif
					continue;
				}
				while ((str = konf_buf_parse(tbuf))) {
					char *answer;
					if (!(answer = process_query(tbuf, conf, str)))
						answer = strdup("-e");
					free(str);
					answer_send(i, answer);
					free(answer);
				}
			}
		}
	}

	/* Free resources */
	konf_tree_delete(conf);

	/* delete each buf */
	while ((tbuf = lub_bintree_findfirst(&bufs))) {
		/* remove the buf from the tree */
		lub_bintree_remove(&bufs, tbuf);
		/* release the instance */
		konf_buf_delete(tbuf);
	}

	retval = 0;
err:
	/* Close RW socket */
	if (sock >= 0) {
		close(sock);
		unlink(opts->socket_path);
	}

	/* Close RO socket */
	if (ro_sock >= 0) {
		close(ro_sock);
		unlink(opts->ro_path);
	}

	/* Remove pidfile */
	if (pidfd >= 0) {
		if (unlink(opts->pidfile) < 0) {
			syslog(LOG_ERR, "Can't remove pid-file %s: %s\n",
			opts->pidfile, strerror(errno));
		}
	}

	/* Free command line options */
	opts_free(opts);

	syslog(LOG_ERR, "Stop daemon.\n");

	return retval;
}

/*--------- Create listen socket--------------------------- */
static int create_listen_socket(const char *path,
	uid_t uid, gid_t gid, mode_t mode)
{
	int sock = -1;
	struct sockaddr_un laddr;
	const int reuseaddr = 1;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "Can't create socket: %s\n", strerror(errno));
		goto err1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		&reuseaddr, sizeof(reuseaddr))) {
		syslog(LOG_ERR, "Can't set socket options: %s\n", strerror(errno));
		goto err1;
	}
	laddr.sun_family = AF_UNIX;
	strncpy(laddr.sun_path, path, USOCK_PATH_MAX);
	laddr.sun_path[USOCK_PATH_MAX - 1] = '\0';
	if (bind(sock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		syslog(LOG_ERR, "Can't bind socket: %s\n", strerror(errno));
		goto err1;
	}
	if (chown(path, uid, gid)) {
		syslog(LOG_ERR, "Can't chown socket: %s\n", strerror(errno));
		goto err2;
	}
	if (chmod(path, mode)) {
		syslog(LOG_ERR, "Can't chmod socket: %s\n", strerror(errno));
		goto err2;
	}
	if (listen(sock, 10)) {
		syslog(LOG_ERR, "Can't listen socket: %s\n", strerror(errno));
		goto err2;
	}

	return sock;

err2:
	unlink(path);
err1:
	if (sock >= 0)
		close(sock);

	return -1;
}

/*--------------------------------------------------------- */
static char * process_query(konf_buf_t *tbuf, konf_tree_t * conf, char *str)
{
	int i;
	int res;
	konf_tree_t *iconf;
	konf_tree_t *tmpconf;
	konf_query_t *query;
	char *retval = NULL;
	konf_query_op_e ret = KONF_QUERY_OP_ERROR;

#ifdef DEBUG
	fprintf(stderr, "REQUEST: %s\n", str);
#endif
	/* Parse query */
	query = konf_query_new();
	res = konf_query_parse_str(query, str);
	if (res < 0) {
		konf_query_free(query);
		return NULL;
	}
#ifdef DEBUG
	konf_query_dump(query);
#endif

	/* Restrict RO socket for non-DUMP operation */
	if (!konf_buf__get_data(tbuf) &&
		(konf_query__get_op(query) != KONF_QUERY_OP_DUMP)) {
#ifdef DEBUG
		fprintf(stderr, "Permission denied. Read-only socket.\n");
#endif
		konf_query_free(query);
		return NULL;
	}

	/* Go through the pwd */
	iconf = conf;
	for (i = 0; i < konf_query__get_pwdc(query); i++) {
		if (!(iconf = konf_tree_find_conf(iconf,
			konf_query__get_pwd(query, i), 0, 0))) {
			iconf = NULL;
			break;
		}
	}

	if (!iconf) {
#ifdef DEBUG
		fprintf(stderr, "Unknown path.\n");
#endif
		konf_query_free(query);
		return NULL;
	}

	switch (konf_query__get_op(query)) {

	case KONF_QUERY_OP_SET:
		if (konf_query__get_unique(query)) {
			int exist = 0;
			exist = konf_tree_del_pattern(iconf,
				konf_query__get_line(query),
				konf_query__get_unique(query),
				konf_query__get_pattern(query),
				konf_query__get_priority(query),
				konf_query__get_seq(query),
				konf_query__get_seq_num(query));
			if (exist < 0)
				break;
			if (exist > 0) {
				ret = KONF_QUERY_OP_OK;
				break;
			}
		}
		tmpconf = konf_tree_new_conf(iconf,
			konf_query__get_line(query), konf_query__get_priority(query),
			konf_query__get_seq(query), konf_query__get_seq_num(query));
		if (!tmpconf)
			break;
		konf_tree__set_splitter(tmpconf, konf_query__get_splitter(query));
		konf_tree__set_depth(tmpconf, konf_query__get_pwdc(query));
		ret = KONF_QUERY_OP_OK;
		break;

	case KONF_QUERY_OP_UNSET:
		if (konf_tree_del_pattern(iconf,
			NULL,
			BOOL_TRUE,
			konf_query__get_pattern(query),
			konf_query__get_priority(query),
			konf_query__get_seq(query),
			konf_query__get_seq_num(query)) < 0)
			break;
		ret = KONF_QUERY_OP_OK;
		break;

	case KONF_QUERY_OP_DUMP:
		if (dump_running_config(konf_buf__get_fd(tbuf), iconf, query))
			break;
		ret = KONF_QUERY_OP_OK;
		break;

	default:
		break;
	}

#ifdef DEBUG
	/* Print whole tree */
	konf_tree_fprintf(conf, stderr, NULL, -1, -1, BOOL_TRUE, 0);
#endif

	/* Free resources */
	konf_query_free(query);

	switch (ret) {
	case KONF_QUERY_OP_OK:
		lub_string_cat(&retval, "-o");
		break;
	case KONF_QUERY_OP_ERROR:
		lub_string_cat(&retval, "-e");
		break;
	default:
		lub_string_cat(&retval, "-e");
		break;
	};

#ifdef DEBUG
	fprintf(stderr, "ANSWER: %s\n", retval);
#endif

	return retval;
}

/*--------------------------------------------------------- */
/*
 * Signal handler for temination signals (like SIGTERM, SIGINT, ...)
 */
static void sighandler(int signo)
{
	signo = signo; /* Happy compiler */

	sigterm = 1;
}

/*--------------------------------------------------------- */
int answer_send(int sock, const char *command)
{
	if (!command) {
		errno = EINVAL;
		return -1;
	}
	return send(sock, command, strlen(command) + 1, MSG_NOSIGNAL);
}

/*--------------------------------------------------------- */
static int dump_running_config(int sock, konf_tree_t *conf, konf_query_t *query)
{
	FILE *fd;
	char *filename;
	int dupsock = -1;

	if ((filename = konf_query__get_path(query))) {
		if (!(fd = fopen(filename, "w")))
			return -1;
	} else {
		if ((dupsock = dup(sock)) < 0)
			return -1;
		fd = fdopen(dupsock, "w");
	}
	if (!filename) {
		fprintf(fd, "-t\n");
#ifdef DEBUG
		fprintf(stderr, "ANSWER: -t\n");
#endif
	}
	konf_tree_fprintf(conf,
		fd,
		konf_query__get_pattern(query),
		konf_query__get_pwdc(query) - 1,
		konf_query__get_depth(query),
		konf_query__get_seq(query),
		0);
	if (!filename) {
		fprintf(fd, "\n");
#ifdef DEBUG
		fprintf(stderr, "SEND DATA: \n");
#endif
	}

	fclose(fd);

	return 0;
}

/*--------------------------------------------------------- */
/* Implement own simple daemon() to don't use Non-POSIX */
int daemonize(int nochdir, int noclose)
{
	int fd;
	int pid;

	pid = fork();
	if (-1 == pid)
		return -1;
	if (pid > 0)
		_exit(0); /* Exit parent */
	if (setsid() == -1)
		return -1;
	if (!nochdir) {
		if (chdir("/"))
			return -1;
	}
	if (!noclose) {
		fd = open("/dev/null", O_RDWR, 0);
		if (fd < 0)
			return -1;
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}

	return 0;
}

/*--------------------------------------------------------- */
/* Initialize option structure by defaults */
struct options *opts_init(void)
{
	struct options *opts = NULL;

	opts = malloc(sizeof(*opts));
	assert(opts);
	opts->debug = 0; /* daemonize by default */
	opts->socket_path = strdup(KONFD_SOCKET_PATH);
	opts->ro_path = NULL;
	opts->pidfile = strdup(KONFD_PIDFILE);
	opts->chroot = NULL;
	opts->uid = getuid();
	opts->gid = getgid();
	opts->log_facility = LOG_DAEMON;

	return opts;
}

/*--------------------------------------------------------- */
/* Free option structure */
void opts_free(struct options *opts)
{
	if (opts->socket_path)
		free(opts->socket_path);
	if (opts->ro_path)
		free(opts->ro_path);
	if (opts->pidfile)
		free(opts->pidfile);
	if (opts->chroot)
		free(opts->chroot);
	free(opts);
}

/*--------------------------------------------------------- */
/* Parse command line options */
static int opts_parse(int argc, char *argv[], struct options *opts)
{
	static const char *shortopts = "hvs:S:p:u:g:dr:O:";
#ifdef HAVE_GETOPT_LONG
	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{"socket",	1, NULL, 's'},
		{"ro-socket",	1, NULL, 'S'},
		{"pid",		1, NULL, 'p'},
		{"user",	1, NULL, 'u'},
		{"group",	1, NULL, 'g'},
		{"debug",	0, NULL, 'd'},
		{"chroot",	1, NULL, 'r'},
		{"facility",	1, NULL, 'O'},
		{NULL,		0, NULL, 0}
	};
#endif
	optind = 1;
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
			if (opts->socket_path)
				free(opts->socket_path);
			opts->socket_path = strdup(optarg);
			break;
		case 'S':
			if (opts->ro_path)
				free(opts->ro_path);
			opts->ro_path = strdup(optarg);
			break;
		case 'p':
			if (opts->pidfile)
				free(opts->pidfile);
			opts->pidfile = strdup(optarg);
			break;
		case 'r':
#ifdef HAVE_CHROOT
			if (opts->chroot)
				free(opts->chroot);
			opts->chroot = strdup(optarg);
#else
			fprintf(stderr, "Error: The --chroot option is not supported.\n");
			return -1;
#endif
			break;
		case 'd':
			opts->debug = 1;
			break;
		case 'u': {
#ifdef HAVE_PWD_H
			struct passwd *pwd = getpwnam(optarg);
			if (!pwd) {
				fprintf(stderr, "Error: Can't identify user \"%s\"\n",
					optarg);
				return -1;
			}
			opts->uid = pwd->pw_uid;
#else
			fprintf(stderr, "The --user option is not supported.\n");
			return -1;
#endif
			break;
		}
		case 'g': {
#ifdef HAVE_GRP_H
			struct group *grp = getgrnam(optarg);
			if (!grp) {
				fprintf(stderr, "Can't identify group \"%s\"\n",
					optarg);
				return -1;
			}
			opts->gid = grp->gr_gid;
#else
			fprintf(stderr, "The --group option is not supported.\n");
			return -1;
#endif
			break;
		}
		case 'O':
			if (lub_log_facility(optarg, &(opts->log_facility))) {
				fprintf(stderr, "Error: Illegal syslog facility %s.\n", optarg);
				help(-1, argv[0]);
				exit(-1);
			}
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
		printf("Usage: %s [options]\n", name);
		printf("Daemon to store user configuration (i.e. commands). "
			"The part of the klish project.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint version.\n");
		printf("\t-h, --help\tPrint this help.\n");
		printf("\t-d, --debug\tDebug mode. Don't daemonize.\n");
		printf("\t-s <path>, --socket=<path>\tSpecify the UNIX socket "
			"filesystem path to listen on.\n");
		printf("\t-p <path>, --pid=<path>\tFile to save daemon's PID to.\n");
		printf("\t-r <path>, --chroot=<path>\tDirectory to chroot.\n");
		printf("\t-u <user>, --user=<user>\tExecute process as"
			" specified user.\n");
		printf("\t-g <group>, --group=<group>\tExecute process as"
			" specified group.\n");
		printf("\t-O, --facility\tSyslog facility. Default is DAEMON.\n");
	}
}
