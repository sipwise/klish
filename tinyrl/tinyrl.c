/*
 * tinyrl.c
 */

/* make sure we can get fileno() */
#undef __STRICT_ANSI__

/* LIBC HEADERS */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* POSIX HEADERS */
#include <unistd.h>

#include "lub/string.h"

#include "private.h"

/*-------------------------------------------------------- */
static int utf8_wchar(const char *sp, unsigned long *sym_out)
{
	int i = 0;
	int l = 0; /* Number of 0x10 UTF sequence bytes */
	unsigned long sym = 0;
	const unsigned char *p = (const unsigned char *)sp;

	if (sym_out)
		*sym_out = *p;

	if (!*p)
		return 0;

	/* Check for first byte of UTF-8 */
	if (!(*p & 0xc0))
		return 1;

	/* Analyze first byte */
	if ((*p & 0xe0) == 0xc0) {
		l = 1;
		sym = (*p & 0x1f);
	} else if ((*p & 0xf0) == 0xe0) {
		l = 2;
		sym = (*p & 0xf);
	} else if ((*p & 0xf8) == 0xf0) {
		l = 3;
		sym = (*p & 7);
	} else if ((*p & 0xfc) == 0xf8) {
		l = 4;
		sym = (*p & 3);
	} else if ((*p & 0xfe) == 0xfc) {
		l = 5;
		sym = (*p & 1);
	} else {
		return 1;
	}
	p++;

	/* Analyze next UTF-8 bytes */
	for (i = 0; i < l; i++) {
		sym <<= 6;
		/* Check if it's really UTF-8 bytes */
		if ((*p & 0xc0) != 0x80)
			return 1;
		sym |= (*p & 0x3f);
		p++;
	}

	if (sym_out)
		*sym_out = sym;
	return (l + 1);
}

/*-------------------------------------------------------- */
static int utf8_is_cjk(unsigned long sym)
{
	if (sym < 0x1100) /* Speed up for non-CJK chars */
		return 0;

	if (sym >= 0x1100 && sym <= 0x11FF) /* Hangul Jamo */
		return 1;
#if 0
	if (sym >=0x2E80 && sym <= 0x2EFF) /* CJK Radicals Supplement */
		return 1;
	if (sym >=0x2F00 && sym <= 0x2FDF) /* Kangxi Radicals */
		return 1;
	if (sym >= 0x2FF0 && sym <= 0x2FFF) /* Ideographic Description Characters */
		return 1;
	if (sym >= 0x3000 && sym < 0x303F) /* CJK Symbols and Punctuation. The U+303f is half space */
		return 1;
	if (sym >= 0x3040 && sym <= 0x309F) /* Hiragana */
		return 1;
	if (sym >= 0x30A0 && sym <=0x30FF) /* Katakana */
		return 1;
	if (sym >= 0x3100 && sym <=0x312F) /* Bopomofo */
		return 1;
	if (sym >= 0x3130 && sym <= 0x318F) /* Hangul Compatibility Jamo */
		return 1;
	if (sym >= 0x3190 && sym <= 0x319F) /* Kanbun */
		return 1;
	if (sym >= 0x31A0 && sym <= 0x31BF) /* Bopomofo Extended */
		return 1;
	if (sym >= 0x31C0 && sym <= 0x31EF) /* CJK strokes */
		return 1;
	if (sym >= 0x31F0 && sym <= 0x31FF) /* Katakana Phonetic Extensions */
		return 1;
	if (sym >= 0x3200 && sym <= 0x32FF) /* Enclosed CJK Letters and Months */
		return 1;
	if (sym >= 0x3300 && sym <= 0x33FF) /* CJK Compatibility */
		return 1;
	if (sym >= 0x3400 && sym <= 0x4DBF) /* CJK Unified Ideographs Extension A */
		return 1;
	if (sym >= 0x4DC0 && sym <= 0x4DFF) /* Yijing Hexagram Symbols */
		return 1;
	if (sym >= 0x4E00 && sym <= 0x9FFF) /* CJK Unified Ideographs */
		return 1;
	if (sym >= 0xA000 && sym <= 0xA48F) /* Yi Syllables */
		return 1;
	if (sym >= 0xA490 && sym <= 0xA4CF) /* Yi Radicals */
		return 1;
#endif
	/* Speed up previous block */
	if (sym >= 0x2E80 && sym <= 0xA4CF && sym != 0x303F)
		return 1;

	if (sym >= 0xAC00 && sym <= 0xD7AF) /* Hangul Syllables */
		return 1;
	if (sym >= 0xF900 && sym <= 0xFAFF) /* CJK Compatibility Ideographs */
		return 1;
	if (sym >= 0xFE10 && sym <= 0xFE1F) /* Vertical Forms */
		return 1;

#if 0
	if (sym >= 0xFE30 && sym <= 0xFE4F) /* CJK Compatibility Forms */
		return 1;
	if (sym >= 0xFE50 && sym <= 0xFE6F) /* Small Form Variants */
		return 1;
#endif
	/* Speed up previous block */
	if (sym >= 0xFE30 && sym <= 0xFE6F)
		return 1;

	if ((sym >= 0xFF00 && sym <= 0xFF60) ||
		(sym >= 0xFFE0 && sym <= 0xFFE6)) /* Fullwidth Forms */
		return 1;

	if (sym >= 0x1D300 && sym <= 0x1D35F) /* Tai Xuan Jing Symbols */
		return 1;
	if (sym >= 0x20000 && sym <= 0x2B81F) /* CJK Unified Ideographs Extensions B, C, D */
		return 1;
	if (sym >= 0x2F800 && sym <= 0x2FA1F) /* CJK Compatibility Ideographs Supplement */
		return 1;

	return 0;
}

/*-------------------------------------------------------- */
static void utf8_point_left(tinyrl_t * this)
{
	if (!this->utf8)
		return;
	while (this->point &&
		(UTF8_10 == (this->line[this->point] & UTF8_MASK)))
		this->point--;
}

/*-------------------------------------------------------- */
static void utf8_point_right(tinyrl_t * this)
{
	if (!this->utf8)
		return;
	while ((this->point < this->end) &&
		(UTF8_10 == (this->line[this->point] & UTF8_MASK)))
		this->point++;
}

/*-------------------------------------------------------- */
static unsigned int utf8_nsyms(const tinyrl_t *this, const char *str,
	unsigned int num)
{
	unsigned int nsym = 0;
	unsigned long sym = 0;
	unsigned int i = 0;

	if (!this->utf8)
		return num;

	while (i < num) {
		if ('\0' == str[i])
			break;
		/* ASCII char */
		if (!(UTF8_7BIT_MASK & str[i])) {
			i++;
			nsym++;
			continue;
		}
		/* Multibyte */
		i += utf8_wchar(&str[i], &sym);
		if (utf8_is_cjk(sym)) /* CJK chars have double-width */
			nsym += 2;
		else
			nsym += 1;
	}

	return nsym;
}

/*----------------------------------------------------------------------- */
static void tty_set_raw_mode(tinyrl_t * this)
{
	struct termios new_termios;
	int fd;

	if (!tinyrl_vt100__get_istream(this->term))
		return;
	fd = fileno(tinyrl_vt100__get_istream(this->term));
	if (tcgetattr(fd, &new_termios) < 0)
		return;
	new_termios.c_iflag = 0;
	new_termios.c_oflag = OPOST | ONLCR;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;
	/* Do the mode switch */
	(void)tcsetattr(fd, TCSADRAIN, &new_termios);
}

/*----------------------------------------------------------------------- */
static void tty_restore_mode(const tinyrl_t * this)
{
	int fd;

	if (!tinyrl_vt100__get_istream(this->term))
		return;
	fd = fileno(tinyrl_vt100__get_istream(this->term));
	/* Do the mode switch */
	(void)tcsetattr(fd, TCSADRAIN, &this->default_termios);
}

/*----------------------------------------------------------------------- */
/*
This is called whenever a line is edited in any way.
It signals that if we are currently viewing a history line we should transfer it
to the current buffer
*/
static void changed_line(tinyrl_t * this)
{
	/* if the current line is not our buffer then make it so */
	if (this->line != this->buffer) {
		/* replace the current buffer with the new details */
		free(this->buffer);
		this->line = this->buffer = lub_string_dup(this->line);
		this->buffer_size = strlen(this->buffer);
		assert(this->line);
	}
}

/*----------------------------------------------------------------------- */
static int tinyrl_timeout_default(tinyrl_t *this)
{
	this = this; /* Happy compiler */

	/* Return -1 to close session on timeout */
	return -1;
}

/*----------------------------------------------------------------------- */
static bool_t tinyrl_key_default(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (key > 31) {
		char tmp[2];
		tmp[0] = (key & 0xFF), tmp[1] = '\0';
		/* inject this text into the buffer */
		result = tinyrl_insert_text(this, tmp);
	} else {
		/* Call the external hotkey analyzer */
		if (this->hotkey_fn)
			this->hotkey_fn(this, key);
	}
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_interrupt(tinyrl_t * this, int key)
{
	tinyrl_crlf(this);
	tinyrl_delete_text(this, 0, this->end);
	this->done = BOOL_TRUE;
	/* keep the compiler happy */
	key = key;

	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_start_of_line(tinyrl_t * this, int key)
{
	/* set the insertion point to the start of the line */
	this->point = 0;
	/* keep the compiler happy */
	key = key;
	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_end_of_line(tinyrl_t * this, int key)
{
	/* set the insertion point to the end of the line */
	this->point = this->end;
	/* keep the compiler happy */
	key = key;
	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_kill(tinyrl_t * this, int key)
{
	/* release any old kill string */
	lub_string_free(this->kill_string);

	/* store the killed string */
	this->kill_string = lub_string_dup(&this->buffer[this->point]);

	/* delete the text to the end of the line */
	tinyrl_delete_text(this, this->point, this->end);
	/* keep the compiler happy */
	key = key;
	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_yank(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->kill_string) {
		/* insert the kill string at the current insertion point */
		result = tinyrl_insert_text(this, this->kill_string);
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_crlf(tinyrl_t * this, int key)
{
	tinyrl_crlf(this);
	this->done = BOOL_TRUE;
	/* keep the compiler happy */
	key = key;
	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_up(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	tinyrl_history_entry_t *entry = NULL;
	if (this->line == this->buffer) {
		/* go to the last history entry */
		entry = tinyrl_history_getlast(this->history, &this->hist_iter);
	} else {
		/* already traversing the history list so get previous */
		entry = tinyrl_history_getprevious(&this->hist_iter);
	}
	if (entry) {
		/* display the entry moving the insertion point
		 * to the end of the line 
		 */
		this->line = tinyrl_history_entry__get_line(entry);
		this->point = this->end = strlen(this->line);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_down(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->line != this->buffer) {
		/* we are not already at the bottom */
		/* the iterator will have been set up by the key_up() function */
		tinyrl_history_entry_t *entry =
		    tinyrl_history_getnext(&this->hist_iter);
		if (!entry) {
			/* nothing more in the history list */
			this->line = this->buffer;
		} else {
			this->line = tinyrl_history_entry__get_line(entry);
		}
		/* display the entry moving the insertion point
		 * to the end of the line 
		 */
		this->point = this->end = strlen(this->line);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_left(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->point > 0) {
		this->point--;
		utf8_point_left(this);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_right(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->point < this->end) {
		this->point++;
		utf8_point_right(this);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_backspace(tinyrl_t *this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->point) {
		unsigned int end = --this->point;
		utf8_point_left(this);
		tinyrl_delete_text(this, this->point, end);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_backword(tinyrl_t *this, int key)
{
	bool_t result = BOOL_FALSE;

    /* remove current whitespace before cursor */
	while (this->point > 0 && isspace(this->line[this->point - 1]))
        tinyrl_key_backspace(this, KEY_BS);

    /* delete word before cusor */
	while (this->point > 0 && !isspace(this->line[this->point - 1]))
        tinyrl_key_backspace(this, KEY_BS);

	result = BOOL_TRUE;

	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_delete(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	if (this->point < this->end) {
		unsigned int begin = this->point++;
		utf8_point_right(this);
		tinyrl_delete_text(this, begin, this->point - 1);
		result = BOOL_TRUE;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_clear_screen(tinyrl_t * this, int key)
{
	tinyrl_vt100_clear_screen(this->term);
	tinyrl_vt100_cursor_home(this->term);
	tinyrl_reset_line_state(this);

	/* keep the compiler happy */
	key = key;
	this = this;
	return BOOL_TRUE;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_erase_line(tinyrl_t * this, int key)
{
	unsigned int end;

	/* release any old kill string */
	lub_string_free(this->kill_string);

	if (!this->point) {
		this->kill_string = NULL;
		return BOOL_TRUE;
	}

	end = this->point - 1;

	/* store the killed string */
	this->kill_string = malloc(this->point + 1);
	memcpy(this->kill_string, this->buffer, this->point);
	this->kill_string[this->point] = '\0';

	/* delete the text from the start of the line */
	tinyrl_delete_text(this, 0, end);
	this->point = 0;

	/* keep the compiler happy */
	key = key;
	this = this;

	return BOOL_TRUE;
}/*-------------------------------------------------------- */

static bool_t tinyrl_escape_seq(tinyrl_t *this, const char *esc_seq)
{
	int key = 0;
	bool_t result = BOOL_FALSE;

	switch (tinyrl_vt100_escape_decode(this->term, esc_seq)) {
	case tinyrl_vt100_CURSOR_UP:
		result = tinyrl_key_up(this, key);
		break;
	case tinyrl_vt100_CURSOR_DOWN:
		result = tinyrl_key_down(this, key);
		break;
	case tinyrl_vt100_CURSOR_LEFT:
		result = tinyrl_key_left(this, key);
		break;
	case tinyrl_vt100_CURSOR_RIGHT:
		result = tinyrl_key_right(this, key);
		break;
	case tinyrl_vt100_HOME:
		result = tinyrl_key_start_of_line(this,key);
		break;
	case tinyrl_vt100_END:
		result = tinyrl_key_end_of_line(this,key);
		break;
	case tinyrl_vt100_DELETE:
		result = tinyrl_key_delete(this,key);
		break;
	case tinyrl_vt100_INSERT:
	case tinyrl_vt100_PGDOWN:
	case tinyrl_vt100_PGUP:
	case tinyrl_vt100_UNKNOWN:
		break;
	}

	return result;
}

/*-------------------------------------------------------- */
static bool_t tinyrl_key_tab(tinyrl_t * this, int key)
{
	bool_t result = BOOL_FALSE;
	tinyrl_match_e status = tinyrl_complete_with_extensions(this);

	switch (status) {
	case TINYRL_COMPLETED_MATCH:
	case TINYRL_MATCH:
		/* everything is OK with the world... */
		result = tinyrl_insert_text(this, " ");
		break;
	case TINYRL_NO_MATCH:
	case TINYRL_MATCH_WITH_EXTENSIONS:
	case TINYRL_AMBIGUOUS:
	case TINYRL_COMPLETED_AMBIGUOUS:
		/* oops don't change the result and let the bell ring */
		break;
	}
	/* keep the compiler happy */
	key = key;
	return result;
}

/*-------------------------------------------------------- */
static void tinyrl_fini(tinyrl_t * this)
{
	/* delete the history session */
	tinyrl_history_delete(this->history);

	/* delete the terminal session */
	tinyrl_vt100_delete(this->term);

	/* free up any dynamic strings */
	lub_string_free(this->buffer);
	lub_string_free(this->kill_string);
	lub_string_free(this->last_buffer);
	lub_string_free(this->prompt);
}

/*-------------------------------------------------------- */
static void tinyrl_init(tinyrl_t * this, FILE * istream, FILE * ostream,
	unsigned int stifle, tinyrl_completion_func_t * complete_fn)
{
	int i;

	for (i = 0; i < NUM_HANDLERS; i++) {
		this->handlers[i] = tinyrl_key_default;
	}
	/* Default handlers */
	this->handlers[KEY_CR] = tinyrl_key_crlf;
	this->handlers[KEY_LF] = tinyrl_key_crlf;
	this->handlers[KEY_ETX] = tinyrl_key_interrupt;
	this->handlers[KEY_DEL] = tinyrl_key_backspace;
	this->handlers[KEY_BS] = tinyrl_key_backspace;
	this->handlers[KEY_EOT] = tinyrl_key_delete;
	this->handlers[KEY_FF] = tinyrl_key_clear_screen;
	this->handlers[KEY_NAK] = tinyrl_key_erase_line;
	this->handlers[KEY_SOH] = tinyrl_key_start_of_line;
	this->handlers[KEY_ENQ] = tinyrl_key_end_of_line;
	this->handlers[KEY_VT] = tinyrl_key_kill;
	this->handlers[KEY_EM] = tinyrl_key_yank;
	this->handlers[KEY_HT] = tinyrl_key_tab;
	this->handlers[KEY_ETB] = tinyrl_key_backword;

	this->line = NULL;
	this->max_line_length = 0;
	this->prompt = NULL;
	this->prompt_size = 0;
	this->buffer = NULL;
	this->buffer_size = 0;
	this->done = BOOL_FALSE;
	this->completion_over = BOOL_FALSE;
	this->point = 0;
	this->end = 0;
	this->attempted_completion_function = complete_fn;
	this->timeout_fn = tinyrl_timeout_default;
	this->keypress_fn = NULL;
	this->hotkey_fn = NULL;
	this->state = 0;
	this->kill_string = NULL;
	this->echo_char = '\0';
	this->echo_enabled = BOOL_TRUE;
	this->last_buffer = NULL;
	this->last_point = 0;
	this->last_line_size = 0;
	this->utf8 = BOOL_FALSE;

	/* create the vt100 terminal */
	this->term = tinyrl_vt100_new(NULL, ostream);
	tinyrl__set_istream(this, istream);
	this->last_width = tinyrl_vt100__get_width(this->term);

	/* create the history */
	this->history = tinyrl_history_new(stifle);
}

/*-------------------------------------------------------- */
int tinyrl_printf(const tinyrl_t * this, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = tinyrl_vt100_vprintf(this->term, fmt, args);
	va_end(args);

	return len;
}

/*-------------------------------------------------------- */
void tinyrl_delete(tinyrl_t * this)
{
	assert(this);
	if (this) {
		/* let the object tidy itself up */
		tinyrl_fini(this);

		/* release the memory associate with this instance */
		free(this);
	}
}

/*-------------------------------------------------------- */

/*#####################################
 * EXPORTED INTERFACE
 *##################################### */
/*----------------------------------------------------------------------- */
int tinyrl_getchar(const tinyrl_t * this)
{
	return tinyrl_vt100_getchar(this->term);
}

/*----------------------------------------------------------------------- */
static void tinyrl_internal_print(const tinyrl_t * this, const char *text)
{
	if (this->echo_enabled) {
		/* simply echo the line */
		tinyrl_vt100_printf(this->term, "%s", text);
	} else {
		/* replace the line with echo char if defined */
		if (this->echo_char) {
			unsigned int i = strlen(text);
			while (i--) {
				tinyrl_vt100_printf(this->term, "%c",
					this->echo_char);
			}
		}
	}
}

/*----------------------------------------------------------------------- */
static void tinyrl_internal_position(const tinyrl_t *this, int prompt_len,
	int line_len, unsigned int width)
{
	int rows, cols;

	rows = ((line_len + prompt_len) / width) - (prompt_len / width);
	cols = ((line_len + prompt_len) % width) - (prompt_len % width);
	if (cols > 0)
		tinyrl_vt100_cursor_back(this->term, cols);
	else if (cols < 0)
		tinyrl_vt100_cursor_forward(this->term, -cols);
	if (rows > 0)
		tinyrl_vt100_cursor_up(this->term, rows);
	else if (rows < 0)
		tinyrl_vt100_cursor_down(this->term, -rows);
}

/*-------------------------------------------------------- */
/* Jump to first free line after current multiline input   */
void tinyrl_multi_crlf(const tinyrl_t * this)
{
	unsigned int line_size = strlen(this->last_buffer);
	unsigned int line_len = utf8_nsyms(this, this->last_buffer, line_size);
	unsigned int count = utf8_nsyms(this, this->last_buffer, this->last_point);

	tinyrl_internal_position(this, this->prompt_len + line_len,
		- (line_len - count), this->last_width);
	tinyrl_crlf(this);
	tinyrl_vt100_oflush(this->term);
}

/*----------------------------------------------------------------------- */
void tinyrl_redisplay(tinyrl_t * this)
{
	unsigned int line_size = strlen(this->line);
	unsigned int line_len = utf8_nsyms(this, this->line, line_size);
	unsigned int width = tinyrl_vt100__get_width(this->term);
	unsigned int count, eq_chars = 0;
	int cols;

	/* Prepare print position */
	if (this->last_buffer && (width == this->last_width)) {
		unsigned int eq_len = 0;
		/* If line and last line have the equal chars at begining */
		eq_chars = lub_string_equal_part(this->line, this->last_buffer,
			this->utf8);
		eq_len = utf8_nsyms(this, this->last_buffer, eq_chars);
		count = utf8_nsyms(this, this->last_buffer, this->last_point);
		tinyrl_internal_position(this, this->prompt_len + eq_len,
			count - eq_len, width);
	} else {
		/* Prepare to resize */
		if (width != this->last_width) {
			tinyrl_vt100_next_line(this->term);
			tinyrl_vt100_erase_down(this->term);
		}
		tinyrl_vt100_printf(this->term, "%s", this->prompt);
	}

	/* Print current line */
	tinyrl_internal_print(this, this->line + eq_chars);
	cols = (this->prompt_len + line_len) % width;
	if (!cols && (line_size - eq_chars))
		tinyrl_vt100_next_line(this->term);
	/* Erase down if current line is shorter than previous one */
	if (this->last_line_size > line_size)
		tinyrl_vt100_erase_down(this->term);
	/* Move the cursor to the insertion point */
	if (this->point < line_size) {
		unsigned int pre_len = utf8_nsyms(this,
			this->line, this->point);
		count = utf8_nsyms(this, this->line + this->point,
			line_size - this->point);
		tinyrl_internal_position(this, this->prompt_len + pre_len,
			count, width);
	}

	/* Update the display */
	tinyrl_vt100_oflush(this->term);

	/* Save the last line buffer */
	lub_string_free(this->last_buffer);
	this->last_buffer = lub_string_dup(this->line);
	this->last_point = this->point;
	this->last_width = width;
	this->last_line_size = line_size;
}

/*----------------------------------------------------------------------- */
tinyrl_t *tinyrl_new(FILE * istream, FILE * ostream,
	unsigned int stifle, tinyrl_completion_func_t * complete_fn)
{
	tinyrl_t *this = NULL;

	this = malloc(sizeof(tinyrl_t));
	if (this)
		tinyrl_init(this, istream, ostream, stifle, complete_fn);

	return this;
}

/*----------------------------------------------------------------------- */
static char *internal_insertline(tinyrl_t * this, char *buffer)
{
	char *p;
	char *s = buffer;

	/* strip any spurious '\r' or '\n' */
	if ((p = strchr(buffer, '\r')))
		*p = '\0';
	if ((p = strchr(buffer, '\n')))
		*p = '\0';
	/* skip any whitespace at the beginning of the line */
	if (0 == this->point) {
		while (*s && isspace(*s))
			s++;
	}
	if (*s) {
		/* append this string to the input buffer */
		(void)tinyrl_insert_text(this, s);
	}
	/* echo the command to the output stream */
	tinyrl_redisplay(this);

	return s;
}

/*----------------------------------------------------------------------- */
static char *internal_readline(tinyrl_t * this,
	void *context, const char *str)
{
	FILE *istream = tinyrl_vt100__get_istream(this->term);
	char *result = NULL;
	int lerrno = 0;

	this->done = BOOL_FALSE;
	this->point = 0;
	this->end = 0;
	this->buffer = lub_string_dup("");
	this->buffer_size = strlen(this->buffer);
	this->line = this->buffer;
	this->context = context;

	/* Interactive session */
	if (this->isatty && !str) {
		unsigned int utf8_cont = 0; /* UTF-8 continue bytes */
		unsigned int esc_cont = 0; /* Escape sequence continues */
		char esc_seq[10]; /* Buffer for ESC sequence */
		char *esc_p = esc_seq;

		/* Set the terminal into raw mode */
		tty_set_raw_mode(this);
		tinyrl_reset_line_state(this);

		while (!this->done) {
			int key;

			key = tinyrl_getchar(this);

			/* Error || EOF || Timeout */
			if (key < 0) {
				if ((VT100_TIMEOUT == key) &&
					!this->timeout_fn(this))
					continue;
				/* It's time to finish the session */
				this->done = BOOL_TRUE;
				this->line = NULL;
				lerrno = ENOENT;
				continue;
			}

			/* Real key pressed */
			/* Common callback for any key */
			if (this->keypress_fn)
				this->keypress_fn(this, key);

			/* Check for ESC sequence. It's a special case. */
			if (!esc_cont && (key == KEY_ESC)) {
				esc_cont = 1; /* Start ESC sequence */
				esc_p = esc_seq;
				continue;
			}
			if (esc_cont) {
				/* Broken sequence */
				if (esc_p >= (esc_seq + sizeof(esc_seq) - 1)) {
					esc_cont = 0;
					continue;
				}
				/* Dump the control sequence into sequence buffer
				   ANSI standard control sequences will end
				   with a character between 64 - 126 */
				*esc_p = key & 0xff;
				esc_p++;
				/* This is an ANSI control sequence terminator code */
				if ((key != '[') && (key > 63)) {
					*esc_p = '\0';
					tinyrl_escape_seq(this, esc_seq);
					esc_cont = 0;
					tinyrl_redisplay(this);
				}
				continue;
			}

			/* Call the handler for this key */
			if (!this->handlers[key](this, key))
				tinyrl_ding(this);
			if (this->done) /* Some handler set the done flag */
				continue; /* It will break the loop */

			if (this->utf8) {
				if (!(UTF8_7BIT_MASK & key)) /* ASCII char */
					utf8_cont = 0;
				else if (utf8_cont && (UTF8_10 == (key & UTF8_MASK))) /* Continue byte */
					utf8_cont--;
				else if (UTF8_11 == (key & UTF8_MASK)) { /* First byte of multibyte char */
					/* Find out number of char's bytes */
					int b = key;
					utf8_cont = 0;
					while ((utf8_cont < 6) && (UTF8_10 != (b & UTF8_MASK))) {
						utf8_cont++;
						b = b << 1;
					}
				}
			}
			/* For non UTF-8 encoding the utf8_cont is always 0.
			   For UTF-8 it's 0 when one-byte symbol or we get
			   all bytes for the current multibyte character. */
			if (!utf8_cont)
				tinyrl_redisplay(this);
		}
		/* If the last character in the line (other than NULL)
		   is a space remove it. */
		if (this->end && this->line && isspace(this->line[this->end - 1]))
			tinyrl_delete_text(this, this->end - 1, this->end);
		/* Restores the terminal mode */
		tty_restore_mode(this);

	/* Non-interactive session */
	} else {
		char *s = NULL, buffer[80];
		size_t len = sizeof(buffer);
		char *tmp = NULL;

		/* manually reset the line state without redisplaying */
		lub_string_free(this->last_buffer);
		this->last_buffer = NULL;

		if (str) {
			tmp = lub_string_dup(str);
			internal_insertline(this, tmp);
		} else {
			while (istream && (sizeof(buffer) == len) &&
				(s = fgets(buffer, sizeof(buffer), istream))) {
				s = internal_insertline(this, buffer);
				len = strlen(buffer) + 1; /* account for the '\0' */
			}
			if (!s || ((this->line[0] == '\0') && feof(istream))) {
				/* time to finish the session */
				this->line = NULL;
				lerrno = ENOENT;
			}
		}

		/*
		 * check against fgets returning null as either error or end of file.
		 * This is a measure to stop potential task spin on encountering an
		 * error from fgets.
		 */
		if (this->line && !this->handlers[KEY_LF](this, KEY_LF)) {
			/* an issue has occured */
			this->line = NULL;
			lerrno = ENOEXEC;
		}
		if (str)
			lub_string_free(tmp);
	}
	/*
	 * duplicate the string for return to the client 
	 * we have to duplicate as we may be referencing a
	 * history entry or our internal buffer
	 */
	result = this->line ? lub_string_dup(this->line) : NULL;

	/* free our internal buffer */
	free(this->buffer);
	this->buffer = NULL;

	if (!result)
		errno = lerrno; /* get saved errno */
	return result;
}

/*----------------------------------------------------------------------- */
char *tinyrl_readline(tinyrl_t * this, void *context)
{
	return internal_readline(this, context, NULL);
}

/*----------------------------------------------------------------------- */
char *tinyrl_forceline(tinyrl_t * this, void *context, const char *line)
{
	return internal_readline(this, context, line);
}

/*----------------------------------------------------------------------- */
/*
 * Ensure that buffer has enough space to hold len characters,
 * possibly reallocating it if necessary. The function returns BOOL_TRUE
 * if the line is successfully extended, BOOL_FALSE if not.
 */
bool_t tinyrl_extend_line_buffer(tinyrl_t * this, unsigned int len)
{
	bool_t result = BOOL_TRUE;
	char *new_buffer;
	size_t new_len = len;

	if (this->buffer_size >= len)
		return result;

	/*
	 * What we do depends on whether we are limited by
	 * memory or a user imposed limit.
	 */
	if (this->max_line_length == 0) {
		/* make sure we don't realloc too often */
		if (new_len < this->buffer_size + 10)
			new_len = this->buffer_size + 10;
		/* leave space for terminator */
		new_buffer = realloc(this->buffer, new_len + 1);

		if (!new_buffer) {
			tinyrl_ding(this);
			result = BOOL_FALSE;
		} else {
			this->buffer_size = new_len;
			this->line = this->buffer = new_buffer;
		}
	} else {
		if (new_len < this->max_line_length) {

			/* Just reallocate once to the max size */
			new_buffer = realloc(this->buffer,
				this->max_line_length);

			if (!new_buffer) {
				tinyrl_ding(this);
				result = BOOL_FALSE;
			} else {
				this->buffer_size =
					this->max_line_length - 1;
				this->line = this->buffer = new_buffer;
			}
		} else {
			tinyrl_ding(this);
			result = BOOL_FALSE;
		}
	}

	return result;
}

/*----------------------------------------------------------------------- */
/*
 * Insert text into the line at the current cursor position.
 */
bool_t tinyrl_insert_text(tinyrl_t * this, const char *text)
{
	unsigned int delta = strlen(text);

	/*
	 * If the client wants to change the line ensure that the line and buffer
	 * references are in sync
	 */
	changed_line(this);

	if ((delta + this->end) > (this->buffer_size)) {
		/* extend the current buffer */
		if (BOOL_FALSE ==
			tinyrl_extend_line_buffer(this, this->end + delta))
			return BOOL_FALSE;
	}

	if (this->point < this->end) {
		/* move the current text to the right (including the terminator) */
		memmove(&this->buffer[this->point + delta],
			&this->buffer[this->point],
			(this->end - this->point) + 1);
	} else {
		/* terminate the string */
		this->buffer[this->end + delta] = '\0';
	}

	/* insert the new text */
	strncpy(&this->buffer[this->point], text, delta);

	/* now update the indexes */
	this->point += delta;
	this->end += delta;

	return BOOL_TRUE;
}

/*----------------------------------------------------------------------- */
/* 
 * A convenience function for displaying a list of strings in columnar
 * format on Readline's output stream. matches is the list of strings,
 * in argv format, such as a list of completion matches. len is the number
 * of strings in matches, and max is the length of the longest string in matches.
 * This function uses the setting of print-completions-horizontally to select
 * how the matches are displayed
 */
void tinyrl_display_matches(const tinyrl_t *this,
	char *const *matches, unsigned int len, size_t max)
{
	unsigned int width = tinyrl_vt100__get_width(this->term);
	unsigned int cols, rows;

	/* Find out column and rows number */
	if (max < width)
		cols = (width + 1) / (max + 1); /* allow for a space between words */
	else
		cols = 1;
	rows = len / cols + 1;

	assert(matches);
	if (matches) {
		unsigned int r, c;
		len--, matches++; /* skip the subtitution string */
		/* Print out a table of completions */
		for (r = 0; r < rows && len; r++) {
			for (c = 0; c < cols && len; c++) {
				const char *match = *matches++;
				len--;
				if ((c + 1) == cols) /* Last str in row */
					tinyrl_vt100_printf(this->term, "%s",
						match);
				else
					tinyrl_vt100_printf(this->term, "%-*s ",
						max, match);
			}
			tinyrl_crlf(this);
		}
	}
}

/*----------------------------------------------------------------------- */
/*
 * Delete the text between start and end in the current line. (inclusive)
 * This adjusts the rl_point and rl_end indexes appropriately.
 */
void tinyrl_delete_text(tinyrl_t * this, unsigned int start, unsigned int end)
{
	unsigned int delta;

	/*
	 * If the client wants to change the line ensure that the line and buffer
	 * references are in sync
	 */
	changed_line(this);

	/* make sure we play it safe */
	if (start > end) {
		unsigned int tmp = end;
		start = end;
		end = tmp;
	}
	if (end > this->end)
		end = this->end;

	delta = (end - start) + 1;

	/* move any text which is left */
	memmove(&this->buffer[start],
		&this->buffer[start + delta], this->end - end);

	/* now adjust the indexs */
	if (this->point >= start) {
		if (this->point > end) {
			/* move the insertion point back appropriately */
			this->point -= delta;
		} else {
			/* move the insertion point to the start */
			this->point = start;
		}
	}
	if (this->end > end)
		this->end -= delta;
	else
		this->end = start;
	/* put a terminator at the end of the buffer */
	this->buffer[this->end] = '\0';
}

/*----------------------------------------------------------------------- */
bool_t tinyrl_bind_key(tinyrl_t * this, int key, tinyrl_key_func_t * fn)
{
	bool_t result = BOOL_FALSE;

	if ((key >= 0) && (key < 256)) {
		/* set the key handling function */
		this->handlers[key] = fn;
		result = BOOL_TRUE;
	}

	return result;
}

/*-------------------------------------------------------- */
/*
 * Returns an array of strings which is a list of completions for text.
 * If there are no completions, returns NULL. The first entry in the
 * returned array is the substitution for text. The remaining entries
 * are the possible completions. The array is terminated with a NULL pointer.
 *
 * entry_func is a function of two args, and returns a char *. 
 * The first argument is text. The second is a state argument;
 * it is zero on the first call, and non-zero on subsequent calls.
 * entry_func returns a NULL pointer to the caller when there are no 
 * more matches. 
 */
char **tinyrl_completion(tinyrl_t * this,
	const char *line, unsigned int start, unsigned int end,
	tinyrl_compentry_func_t * entry_func)
{
	unsigned int state = 0;
	size_t size = 1;
	unsigned int offset = 1; /* Need at least one entry for the substitution */
	char **matches = NULL;
	char *match;
	/* duplicate the string upto the insertion point */
	char *text = lub_string_dupn(line, end);

	/* now try and find possible completions */
	while ((match = entry_func(this, text, start, state++))) {
		if (size == offset) {
			/* resize the buffer if needed - the +1 is for the NULL terminator */
			size += 10;
			matches =
			    realloc(matches, (sizeof(char *) * (size + 1)));
		}
		/* not much we can do... */
		if (!matches)
			break;
		matches[offset] = match;

		/*
		 * augment the substitute string with this entry
		 */
		if (1 == offset) {
			/* let's be optimistic */
			matches[0] = lub_string_dup(match);
		} else {
			char *p = matches[0];
			size_t match_len = strlen(p);
			/* identify the common prefix */
			while ((tolower(*p) == tolower(*match)) && match_len--) {
				p++, match++;
			}
			/* terminate the prefix string */
			*p = '\0';
		}
		offset++;
	}
	/* be a good memory citizen */
	lub_string_free(text);

	if (matches)
		matches[offset] = NULL;
	return matches;
}

/*-------------------------------------------------------- */
void tinyrl_delete_matches(char **this)
{
	char **matches = this;
	while (*matches) {
		/* release the memory for each contained string */
		free(*matches++);
	}
	/* release the memory for the array */
	free(this);
}

/*-------------------------------------------------------- */
void tinyrl_crlf(const tinyrl_t * this)
{
	tinyrl_vt100_printf(this->term, "\n");
}

/*-------------------------------------------------------- */
/*
 * Ring the terminal bell, obeying the setting of bell-style.
 */
void tinyrl_ding(const tinyrl_t * this)
{
	tinyrl_vt100_ding(this->term);
}

/*-------------------------------------------------------- */
void tinyrl_reset_line_state(tinyrl_t * this)
{
	lub_string_free(this->last_buffer);
	this->last_buffer = NULL;
	this->last_line_size = 0;

	tinyrl_redisplay(this);
}

/*-------------------------------------------------------- */
void tinyrl_replace_line(tinyrl_t * this, const char *text, int clear_undo)
{
	size_t new_len = strlen(text);

	/* ignored for now */
	clear_undo = clear_undo;

	/* ensure there is sufficient space */
	if (tinyrl_extend_line_buffer(this, new_len)) {

		/* overwrite the current contents of the buffer */
		strcpy(this->buffer, text);

		/* set the insert point and end point */
		this->point = this->end = new_len;
	}
	tinyrl_redisplay(this);
}

/*-------------------------------------------------------- */
static tinyrl_match_e
tinyrl_do_complete(tinyrl_t * this, bool_t with_extensions)
{
	tinyrl_match_e result = TINYRL_NO_MATCH;
	char **matches = NULL;
	unsigned int start, end;
	bool_t completion = BOOL_FALSE;
	bool_t prefix = BOOL_FALSE;
	int i = 0;

	/* find the start and end of the current word */
	start = end = this->point;
	while (start && !isspace(this->line[start - 1]))
		start--;

	if (this->attempted_completion_function) {
		this->completion_over = BOOL_FALSE;
		this->completion_error_over = BOOL_FALSE;
		/* try and complete the current line buffer */
		matches = this->attempted_completion_function(this,
			this->line, start, end);
	}
	if (!matches && (BOOL_FALSE == this->completion_over)) {
		/* insert default completion call here... */
	}
	if (!matches)
		return result;

	/* identify and insert a common prefix if there is one */
	if (0 != strncmp(matches[0], &this->line[start],
		strlen(matches[0]))) {
		/*
		 * delete the original text not including
		 * the current insertion point character
		 */
		if (this->end != end)
			end--;
		tinyrl_delete_text(this, start, end);
		if (BOOL_FALSE == tinyrl_insert_text(this, matches[0]))
			return TINYRL_NO_MATCH;
		completion = BOOL_TRUE;
	}
	for (i = 1; matches[i]; i++) {
		/* this is just a prefix string */
		if (0 == lub_string_nocasecmp(matches[0], matches[i]))
			prefix = BOOL_TRUE;
	}
	/* is there more than one completion? */
	if (matches[2]) {
		char **tmp = matches;
		unsigned int max, len;
		max = len = 0;
		while (*tmp) {
			size_t size = strlen(*tmp++);
			len++;
			if (size > max)
				max = size;
		}
		if (completion)
			result = TINYRL_COMPLETED_AMBIGUOUS;
		else if (prefix)
			result = TINYRL_MATCH_WITH_EXTENSIONS;
		else
			result = TINYRL_AMBIGUOUS;
		if (with_extensions || !prefix) {
			/* Either we always want to show extensions or
			 * we haven't been able to complete the current line
			 * and there is just a prefix, so let the user see the options
			 */
			tinyrl_crlf(this);
			tinyrl_display_matches(this, matches, len, max);
			tinyrl_reset_line_state(this);
		}
	} else {
		result = completion ?
			TINYRL_COMPLETED_MATCH : TINYRL_MATCH;
	}
	/* free the memory */
	tinyrl_delete_matches(matches);
	/* redisplay the line */
	tinyrl_redisplay(this);

	return result;
}

/*-------------------------------------------------------- */
tinyrl_match_e tinyrl_complete_with_extensions(tinyrl_t * this)
{
	return tinyrl_do_complete(this, BOOL_TRUE);
}

/*-------------------------------------------------------- */
tinyrl_match_e tinyrl_complete(tinyrl_t * this)
{
	return tinyrl_do_complete(this, BOOL_FALSE);
}

/*-------------------------------------------------------- */
void *tinyrl__get_context(const tinyrl_t * this)
{
	return this->context;
}

/*--------------------------------------------------------- */
const char *tinyrl__get_line(const tinyrl_t * this)
{
	return this->line;
}

/*--------------------------------------------------------- */
tinyrl_history_t *tinyrl__get_history(const tinyrl_t * this)
{
	return this->history;
}

/*--------------------------------------------------------- */
void tinyrl_completion_over(tinyrl_t * this)
{
	this->completion_over = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_completion_error_over(tinyrl_t * this)
{
	this->completion_error_over = BOOL_TRUE;
}

/*--------------------------------------------------------- */
bool_t tinyrl_is_completion_error_over(const tinyrl_t * this)
{
	return this->completion_error_over;
}

/*--------------------------------------------------------- */
void tinyrl_done(tinyrl_t * this)
{
	this->done = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_enable_echo(tinyrl_t * this)
{
	this->echo_enabled = BOOL_TRUE;
}

/*--------------------------------------------------------- */
void tinyrl_disable_echo(tinyrl_t * this, char echo_char)
{
	this->echo_enabled = BOOL_FALSE;
	this->echo_char = echo_char;
}

/*--------------------------------------------------------- */
void tinyrl__set_istream(tinyrl_t * this, FILE * istream)
{
	tinyrl_vt100__set_istream(this->term, istream);
	if (istream) {
		int fd;
		this->isatty = isatty(fileno(istream)) ? BOOL_TRUE : BOOL_FALSE;
		/* Save terminal settings to restore on exit */
		fd = fileno(istream);
		tcgetattr(fd, &this->default_termios);
	} else
		this->isatty = BOOL_FALSE;
}

/*-------------------------------------------------------- */
bool_t tinyrl__get_isatty(const tinyrl_t * this)
{
	return this->isatty;
}

/*-------------------------------------------------------- */
FILE *tinyrl__get_istream(const tinyrl_t * this)
{
	return tinyrl_vt100__get_istream(this->term);
}

/*-------------------------------------------------------- */
FILE *tinyrl__get_ostream(const tinyrl_t * this)
{
	return tinyrl_vt100__get_ostream(this->term);
}

/*-------------------------------------------------------- */
const char *tinyrl__get_prompt(const tinyrl_t * this)
{
	return this->prompt;
}

/*-------------------------------------------------------- */
void tinyrl__set_prompt(tinyrl_t *this, const char *prompt)
{
	if (this->prompt) {
		lub_string_free(this->prompt);
		this->prompt_size = 0;
		this->prompt_len = 0;
	}
	this->prompt = lub_string_dup(prompt);
	if (this->prompt) {
		this->prompt_size = strlen(this->prompt);
		this->prompt_len = utf8_nsyms(this, this->prompt,
			this->prompt_size);
	}
}

/*-------------------------------------------------------- */
bool_t tinyrl__get_utf8(const tinyrl_t * this)
{
	return this->utf8;
}

/*-------------------------------------------------------- */
void tinyrl__set_utf8(tinyrl_t * this, bool_t utf8)
{
	this->utf8 = utf8;
}

/*-------------------------------------------------------- */
void tinyrl__set_timeout(tinyrl_t *this, int timeout)
{
	tinyrl_vt100__set_timeout(this->term, timeout);
}

/*-------------------------------------------------------- */
void tinyrl__set_timeout_fn(tinyrl_t *this,
	tinyrl_timeout_fn_t *fn)
{
	this->timeout_fn = fn;
}

/*-------------------------------------------------------- */
void tinyrl__set_keypress_fn(tinyrl_t *this,
	tinyrl_keypress_fn_t *fn)
{
	this->keypress_fn = fn;
}

/*-------------------------------------------------------- */
void tinyrl__set_hotkey_fn(tinyrl_t *this,
	tinyrl_key_func_t *fn)
{
	this->hotkey_fn = fn;
}

/*-------------------------------------------------------- */
bool_t tinyrl_is_quoting(const tinyrl_t * this)
{
	bool_t result = BOOL_FALSE;
	/* count the quotes upto the current insertion point */
	unsigned int i = 0;
	while (i < this->point) {
		if (result && (this->line[i] == '\\')) {
			i++;
			if (i >= this->point)
				break;
			i++;
			continue;
		}
		if (this->line[i++] == '"') {
			result = result ? BOOL_FALSE : BOOL_TRUE;
		}
	}
	return result;
}

/*-------------------------------------------------------- */
bool_t tinyrl_is_empty(const tinyrl_t *this)
{
	return (this->point == 0) ? BOOL_TRUE : BOOL_FALSE;
}

/*--------------------------------------------------------- */
void tinyrl_limit_line_length(tinyrl_t * this, unsigned int length)
{
	this->max_line_length = length;
}

/*--------------------------------------------------------- */
extern unsigned int tinyrl__get_width(const tinyrl_t *this)
{
	return tinyrl_vt100__get_width(this->term);
}

/*--------------------------------------------------------- */
extern unsigned int tinyrl__get_height(const tinyrl_t *this)
{
	return tinyrl_vt100__get_height(this->term);
}

/*----------------------------------------------------------*/
int tinyrl__save_history(const tinyrl_t *this, const char *fname)
{
	return tinyrl_history_save(this->history, fname);
}

/*----------------------------------------------------------*/
int tinyrl__restore_history(tinyrl_t *this, const char *fname)
{
	return tinyrl_history_restore(this->history, fname);
}

/*----------------------------------------------------------*/
void tinyrl__stifle_history(tinyrl_t *this, unsigned int stifle)
{
	tinyrl_history_stifle(this->history, stifle);
}
/*--------------------------------------------------------- */
