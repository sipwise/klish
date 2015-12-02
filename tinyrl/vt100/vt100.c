#undef __STRICT_ANSI__		/* we need to use fileno() */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>

#include "private.h"

typedef struct {
	const char* sequence;
	tinyrl_vt100_escape_e code;
} vt100_decode_t;

/* This table maps the vt100 escape codes to an enumeration */
static vt100_decode_t cmds[] = {
	{"[A", tinyrl_vt100_CURSOR_UP},
	{"[B", tinyrl_vt100_CURSOR_DOWN},
	{"[C", tinyrl_vt100_CURSOR_RIGHT},
	{"[D", tinyrl_vt100_CURSOR_LEFT},
	{"[H", tinyrl_vt100_HOME},
	{"[1~", tinyrl_vt100_HOME},
	{"[F", tinyrl_vt100_END},
	{"[4~", tinyrl_vt100_END},
	{"[2~", tinyrl_vt100_INSERT},
	{"[3~", tinyrl_vt100_DELETE},
	{"[5~", tinyrl_vt100_PGUP},
	{"[6~", tinyrl_vt100_PGDOWN},
};

/*--------------------------------------------------------- */
tinyrl_vt100_escape_e tinyrl_vt100_escape_decode(const tinyrl_vt100_t *this,
	const char *esc_seq)
{
	tinyrl_vt100_escape_e result = tinyrl_vt100_UNKNOWN;
	unsigned int i;

	/* Decode the sequence to macros */
	for (i = 0; i < (sizeof(cmds) / sizeof(vt100_decode_t)); i++) {
		if (strcmp(cmds[i].sequence, esc_seq))
			continue;
		result = cmds[i].code;
		break;
	}

	this = this; /* Happy compiler */

	return result;
}

/*-------------------------------------------------------- */
int tinyrl_vt100_printf(const tinyrl_vt100_t * this, const char *fmt, ...)
{
	va_list args;
	int len;

	if (!this->ostream)
		return 0;
	va_start(args, fmt);
	len = tinyrl_vt100_vprintf(this, fmt, args);
	va_end(args);

	return len;
}

/*-------------------------------------------------------- */
int
tinyrl_vt100_vprintf(const tinyrl_vt100_t * this, const char *fmt, va_list args)
{
	if (!this->ostream)
		return 0;
	return vfprintf(this->ostream, fmt, args);
}

/*-------------------------------------------------------- */
int tinyrl_vt100_getchar(const tinyrl_vt100_t *this)
{
	unsigned char c;
	int istream_fd;
	fd_set rfds;
	struct timeval tv;
	int retval;
	ssize_t res;

	if (!this->istream)
		return VT100_ERR;
	istream_fd = fileno(this->istream);

	/* Just wait for the input if no timeout */
	if (this->timeout <= 0) {
		while (((res = read(istream_fd, &c, 1)) < 0) &&
			(EAGAIN == errno));
		/* EOF or error */
		if (res < 0)
			return VT100_ERR;
		if (!res)
			return VT100_EOF;
		return c;
	}

	/* Set timeout for the select() */
	FD_ZERO(&rfds);
	FD_SET(istream_fd, &rfds);
	tv.tv_sec = this->timeout;
	tv.tv_usec = 0;
	while (((retval = select(istream_fd + 1, &rfds, NULL, NULL, &tv)) < 0) &&
		(EAGAIN == errno));
	/* Error or timeout */
	if (retval < 0)
		return VT100_ERR;
	if (!retval)
		return VT100_TIMEOUT;

	res = read(istream_fd, &c, 1);
	/* EOF or error */
	if (res < 0)
		return VT100_ERR;
	if (!res)
		return VT100_EOF;

	return c;
}

/*-------------------------------------------------------- */
int tinyrl_vt100_oflush(const tinyrl_vt100_t * this)
{
	if (!this->ostream)
		return 0;
	return fflush(this->ostream);
}

/*-------------------------------------------------------- */
int tinyrl_vt100_ierror(const tinyrl_vt100_t * this)
{
	if (!this->istream)
		return 0;
	return ferror(this->istream);
}

/*-------------------------------------------------------- */
int tinyrl_vt100_oerror(const tinyrl_vt100_t * this)
{
	if (!this->ostream)
		return 0;
	return ferror(this->ostream);
}

/*-------------------------------------------------------- */
int tinyrl_vt100_ieof(const tinyrl_vt100_t * this)
{
	if (!this->istream)
		return 0;
	return feof(this->istream);
}

/*-------------------------------------------------------- */
int tinyrl_vt100_eof(const tinyrl_vt100_t * this)
{
	if (!this->istream)
		return 0;
	return feof(this->istream);
}

/*-------------------------------------------------------- */
unsigned int tinyrl_vt100__get_width(const tinyrl_vt100_t *this)
{
#ifdef TIOCGWINSZ
	struct winsize ws;
	int res;
#endif

	if(!this->ostream)
		return 80;

#ifdef TIOCGWINSZ
	ws.ws_col = 0;
	res = ioctl(fileno(this->ostream), TIOCGWINSZ, &ws);
	if (res || !ws.ws_col)
		return 80;
	return ws.ws_col;
#else
	return 80;
#endif
}

/*-------------------------------------------------------- */
unsigned int tinyrl_vt100__get_height(const tinyrl_vt100_t *this)
{
#ifdef TIOCGWINSZ
	struct winsize ws;
	int res;
#endif

	if(!this->ostream)
		return 25;

#ifdef TIOCGWINSZ
	ws.ws_row = 0;
	res = ioctl(fileno(this->ostream), TIOCGWINSZ, &ws);
	if (res || !ws.ws_row)
		return 25;
	return ws.ws_row;
#else
	return 25;
#endif
}

/*-------------------------------------------------------- */
static void
tinyrl_vt100_init(tinyrl_vt100_t * this, FILE * istream, FILE * ostream)
{
	this->istream = istream;
	this->ostream = ostream;
	this->timeout = -1; /* No timeout by default */
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_fini(tinyrl_vt100_t * this)
{
	/* nothing to do yet... */
	this = this;
}

/*-------------------------------------------------------- */
tinyrl_vt100_t *tinyrl_vt100_new(FILE * istream, FILE * ostream)
{
	tinyrl_vt100_t *this = NULL;

	this = malloc(sizeof(tinyrl_vt100_t));
	if (this) {
		tinyrl_vt100_init(this, istream, ostream);
	}

	return this;
}

/*-------------------------------------------------------- */
void tinyrl_vt100_delete(tinyrl_vt100_t * this)
{
	tinyrl_vt100_fini(this);
	/* release the memory */
	free(this);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_ding(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c", KEY_BEL);
	(void)tinyrl_vt100_oflush(this);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_reset(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[0m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_bright(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[1m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_dim(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[2m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_underscore(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[4m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_blink(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[5m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_reverse(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[7m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_attribute_hidden(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[8m", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_erase_line(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[2K", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_clear_screen(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[2J", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_save(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c7", KEY_ESC); /* VT100 */
/*	tinyrl_vt100_printf(this, "%c[s", KEY_ESC); */ /* ANSI */
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_restore(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c8", KEY_ESC); /* VT100 */
/*	tinyrl_vt100_printf(this, "%c[u", KEY_ESC); */ /* ANSI */
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_forward(const tinyrl_vt100_t * this, unsigned count)
{
	tinyrl_vt100_printf(this, "%c[%dC", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_back(const tinyrl_vt100_t * this, unsigned count)
{
	tinyrl_vt100_printf(this, "%c[%dD", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_up(const tinyrl_vt100_t * this, unsigned count)
{
	tinyrl_vt100_printf(this, "%c[%dA", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_down(const tinyrl_vt100_t * this, unsigned count)
{
	tinyrl_vt100_printf(this, "%c[%dB", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_scroll_up(const tinyrl_vt100_t *this)
{
	tinyrl_vt100_printf(this, "%cD", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_scroll_down(const tinyrl_vt100_t *this)
{
	tinyrl_vt100_printf(this, "%cM", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_next_line(const tinyrl_vt100_t *this)
{
	tinyrl_vt100_printf(this, "%cE", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_cursor_home(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[H", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100_erase(const tinyrl_vt100_t * this, unsigned count)
{
	tinyrl_vt100_printf(this, "%c[%dP", KEY_ESC, count);
}

/*-------------------------------------------------------- */
void tinyrl_vt100__set_timeout(tinyrl_vt100_t *this, int timeout)
{
	this->timeout = timeout;
}

/*-------------------------------------------------------- */
void tinyrl_vt100_erase_down(const tinyrl_vt100_t * this)
{
	tinyrl_vt100_printf(this, "%c[J", KEY_ESC);
}

/*-------------------------------------------------------- */
void tinyrl_vt100__set_istream(tinyrl_vt100_t * this, FILE * istream)
{
	this->istream = istream;
}

/*-------------------------------------------------------- */
FILE *tinyrl_vt100__get_istream(const tinyrl_vt100_t * this)
{
	return this->istream;
}

/*-------------------------------------------------------- */
FILE *tinyrl_vt100__get_ostream(const tinyrl_vt100_t * this)
{
	return this->ostream;
}

/*-------------------------------------------------------- */
