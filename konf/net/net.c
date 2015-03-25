#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <fcntl.h>

#include "konf/buf.h"
#include "konf/query.h"
#include "lub/string.h"
#include "private.h"

/* UNIX socket name in filesystem */
/* Don't use UNIX_PATH_MAX due to portability issues */
#define USOCK_PATH_MAX sizeof(((struct sockaddr_un *)0)->sun_path)

/* OpenBSD has no MSG_NOSIGNAL flag.
 * The SIGPIPE must be ignored in application.
 */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/*--------------------------------------------------------- */
konf_client_t *konf_client_new(const char *path)
{
	konf_client_t *this;

	if (!path)
		return NULL;

	if (!(this = malloc(sizeof(*this))))
		return NULL;

	this->sock = -1; /* socket is not created yet */
	this->path = strdup(path);

	return this;
}

/*--------------------------------------------------------- */
void konf_client_free(konf_client_t *this)
{
	if (!this)
		return;
	if (this->sock != -1)
		konf_client_disconnect(this);
	free(this->path);

	free(this);
}

/*--------------------------------------------------------- */
int konf_client_connect(konf_client_t *this)
{
	struct sockaddr_un raddr;

	if (this->sock >= 0)
		return this->sock;

	if ((this->sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return this->sock;

#ifdef FD_CLOEXEC
	fcntl(this->sock, F_SETFD, fcntl(this->sock, F_GETFD) | FD_CLOEXEC);
#endif

	raddr.sun_family = AF_UNIX;
	strncpy(raddr.sun_path, this->path, USOCK_PATH_MAX);
	raddr.sun_path[USOCK_PATH_MAX - 1] = '\0';
	if (connect(this->sock, (struct sockaddr *)&raddr, sizeof(raddr))) {
		close(this->sock);
		this->sock = -1;
	}

	return this->sock;
}

/*--------------------------------------------------------- */
void konf_client_disconnect(konf_client_t *this)
{
	if (this->sock >= 0) {
		close(this->sock);
		this->sock = -1;
	}
}

/*--------------------------------------------------------- */
int konf_client_reconnect(konf_client_t *this)
{
	konf_client_disconnect(this);
	return konf_client_connect(this);
}

/*--------------------------------------------------------- */
int konf_client_send(konf_client_t *this, char *command)
{
	if (this->sock < 0)
		return this->sock;

	return send(this->sock, command, strlen(command) + 1, MSG_NOSIGNAL);
}

/*--------------------------------------------------------- */
int konf_client__get_sock(konf_client_t *this)
{
	return this->sock;
}

/*--------------------------------------------------------- */
konf_buf_t * konf_client_recv_data(konf_client_t * this, konf_buf_t *buf)
{
	int processed = 0;
	konf_buf_t *data;
	char *str;

	/* Check if socked is connected */
	if ((konf_client_connect(this) < 0))
		return NULL;

	data = konf_buf_new(konf_client__get_sock(this));
	do {
		while ((str = konf_buf_parse(buf))) {
			konf_buf_add(data, str, strlen(str) + 1);
			if (strlen(str) == 0) {
				processed = 1;
				free(str);
				break;
			}
			free(str);
		}
	} while ((!processed) && (konf_buf_read(buf)) > 0);
	if (!processed) {
		konf_buf_delete(data);
		return NULL;
	}

	return data;
}

/*--------------------------------------------------------- */
static int process_answer(konf_client_t * this, char *str, konf_buf_t *buf, konf_buf_t **data)
{
	int res;
	konf_query_t *query;

	/* Parse query */
	query = konf_query_new();
	res = konf_query_parse_str(query, str);
	if (res < 0) {
		konf_query_free(query);
#ifdef DEBUG
		fprintf(stderr, "CONFIG error: Cannot parse answer string.\n");
#endif
		return -1;
	}

#ifdef DEBUG
	fprintf(stderr, "ANSWER: %s\n", str);
/*	konf_query_dump(query);
*/
#endif
	switch (konf_query__get_op(query)) {
	case KONF_QUERY_OP_OK:
		res = 0;
		break;
	case KONF_QUERY_OP_ERROR:
		res = -1;
		break;
	case KONF_QUERY_OP_STREAM:
		if (!(*data = konf_client_recv_data(this, buf)))
			res = -1;
		else
			res = 1; /* wait for another answer */
		break;
	default:
		res = -1;
		break;
	}

	/* Free resources */
	konf_query_free(query);

	return res;
}

/*--------------------------------------------------------- */
int konf_client_recv_answer(konf_client_t * this, konf_buf_t **data)
{
	konf_buf_t *buf;
	int nbytes;
	char *str;
	int retval = 0;
	int processed = 0;

	if ((konf_client_connect(this) < 0))
		return -1;

	buf = konf_buf_new(konf_client__get_sock(this));
	while ((!processed) && (nbytes = konf_buf_read(buf)) > 0) {
		while ((str = konf_buf_parse(buf))) {
			konf_buf_t *tmpdata = NULL;
			retval = process_answer(this, str, buf, &tmpdata);
			free(str);
			if (retval < 0) {
				konf_buf_delete(buf);
				return retval;
			}
			if (retval == 0)
				processed = 1;
			if (tmpdata) {
				if (*data)
					konf_buf_delete(*data);
				*data = tmpdata;
			}
		}
	}
	konf_buf_delete(buf);

	return retval;
}

