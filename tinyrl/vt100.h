/*
 * vt100.h
 *
 * A simple class representing a vt100 terminal
 */
  /**
\ingroup tinyrl
\defgroup tinyrl_vt100 vt100
@{

\brief A simple class for controlling and interacting with a VT100 compatible
terminal.

This class has been implemented pragmatically in an as needed fashion, so 
doesn't support all the features of a VT100 terminal.

*/

#ifndef _tinyrl_vt100_h
#define _tinyrl_vt100_h

#include <stdio.h>
#include <stdarg.h>

#include "lub/c_decl.h"
#include "lub/types.h"

_BEGIN_C_DECL typedef struct _tinyrl_vt100 tinyrl_vt100_t;

/* define the Key codes */
#define KEY_NUL	0	/**< ^@	Null character */
#define KEY_SOH	1	/**< ^A	Start of heading, = console interrupt */
#define KEY_STX	2	/**< ^B	Start of text, maintenance mode on HP console */
#define KEY_ETX	3	/**< ^C	End of text */
#define KEY_EOT	4	/**< ^D	End of transmission, not the same as ETB */
#define KEY_ENQ	5	/**< ^E	Enquiry, goes with ACK; old HP flow control */
#define KEY_ACK	6	/**< ^F	Acknowledge, clears ENQ logon hand */
#define KEY_BEL	7	/**< ^G	Bell, rings the bell... */
#define KEY_BS	8	/**< ^H	Backspace, works on HP terminals/computers */
#define KEY_HT	9	/**< ^I	Horizontal tab, move to next tab stop */
#define KEY_LF	10	/**< ^J	Line Feed */
#define KEY_VT	11	/**< ^K	Vertical tab */
#define KEY_FF	12	/**< ^L	Form Feed, page eject */
#define KEY_CR	13	/**< ^M	Carriage Return*/
#define KEY_SO	14	/**< ^N	Shift Out, alternate character set */
#define KEY_SI	15	/**< ^O	Shift In, resume defaultn character set */
#define KEY_DLE	16	/**< ^P	Data link escape */
#define KEY_DC1	17	/**< ^Q	XON, with XOFF to pause listings; "okay to send". */
#define KEY_DC2	18	/**< ^R	Device control 2, block-mode flow control */
#define KEY_DC3	19	/**< ^S	XOFF, with XON is TERM=18 flow control */
#define KEY_DC4	20	/**< ^T	Device control 4 */
#define KEY_NAK	21	/**< ^U	Negative acknowledge */
#define KEY_SYN	22	/**< ^V	Synchronous idle */
#define KEY_ETB	23	/**< ^W	End transmission block, not the same as EOT */
#define KEY_CAN	24	/**< ^X	Cancel line, MPE echoes !!! */
#define KEY_EM	25	/**< ^Y	End of medium, Control-Y interrupt */
#define KEY_SUB	26	/**< ^Z	Substitute */
#define KEY_ESC	27	/**< ^[	Escape, next character is not echoed */
#define KEY_FS	28	/**< ^\	File separator */
#define KEY_GS	29	/**< ^]	Group separator */
#define KEY_RS	30	/**< ^^	Record separator, block-mode terminator */
#define KEY_US	31	/**< ^_	Unit separator */

#define KEY_DEL 127 /**< Delete (not a real control character...) */

/**
 * This enumeration is used to identify the types of escape code 
 */
typedef enum {
	tinyrl_vt100_UNKNOWN, /**< Undefined escape sequence */
	tinyrl_vt100_CURSOR_UP, /**< Move the cursor up */
	tinyrl_vt100_CURSOR_DOWN, /**< Move the cursor down */
	tinyrl_vt100_CURSOR_LEFT, /**< Move the cursor left */
	tinyrl_vt100_CURSOR_RIGHT, /**< Move the cursor right */
	tinyrl_vt100_HOME, /**< Move the cursor to the beginning of the line */
	tinyrl_vt100_END, /**< Move the cursor to the end of the line */
	tinyrl_vt100_INSERT, /**< No action at the moment */
	tinyrl_vt100_DELETE, /**< Delete character on the right */
	tinyrl_vt100_PGUP, /**< No action at the moment */
	tinyrl_vt100_PGDOWN /**< No action at the moment */
} tinyrl_vt100_escape_e;

/* Return values from vt100_getchar() */
#define VT100_EOF	-1
#define VT100_TIMEOUT	-2
#define VT100_ERR	-3

extern tinyrl_vt100_t *tinyrl_vt100_new(FILE * instream, FILE * outstream);
extern void tinyrl_vt100_delete(tinyrl_vt100_t * instance);

/*lint -esym(534,tinyrl_vt100_printf) Ignoring return value of function */
extern int tinyrl_vt100_printf(const tinyrl_vt100_t * instance, const char *fmt, ...
    );
extern int
tinyrl_vt100_vprintf(const tinyrl_vt100_t * instance,
		     const char *fmt, va_list args);

extern int tinyrl_vt100_oflush(const tinyrl_vt100_t * instance);
extern int tinyrl_vt100_ierror(const tinyrl_vt100_t * instance);
extern int tinyrl_vt100_oerror(const tinyrl_vt100_t * instance);
extern int tinyrl_vt100_ieof(const tinyrl_vt100_t * instance);
extern int tinyrl_vt100_getchar(const tinyrl_vt100_t * instance);
extern unsigned tinyrl_vt100__get_width(const tinyrl_vt100_t * instance);
extern unsigned tinyrl_vt100__get_height(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100__set_timeout(tinyrl_vt100_t *instance, int timeout);
extern void
tinyrl_vt100__set_istream(tinyrl_vt100_t * instance, FILE * istream);
extern FILE *tinyrl_vt100__get_istream(const tinyrl_vt100_t * instance);
extern FILE *tinyrl_vt100__get_ostream(const tinyrl_vt100_t * instance);

extern tinyrl_vt100_escape_e
tinyrl_vt100_escape_decode(const tinyrl_vt100_t * instance, const char *esc_seq);
extern void tinyrl_vt100_ding(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_reset(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_bright(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_dim(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_underscore(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_blink(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_reverse(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_attribute_hidden(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_erase_line(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_clear_screen(const tinyrl_vt100_t * instance);
extern void
tinyrl_vt100_cursor_back(const tinyrl_vt100_t * instance, unsigned count);
extern void
tinyrl_vt100_cursor_forward(const tinyrl_vt100_t * instance, unsigned count);
extern void
tinyrl_vt100_cursor_up(const tinyrl_vt100_t * instance, unsigned count);
extern void
tinyrl_vt100_cursor_down(const tinyrl_vt100_t * instance, unsigned count);
extern void tinyrl_vt100_scroll_up(const tinyrl_vt100_t *instance);
extern void tinyrl_vt100_scroll_down(const tinyrl_vt100_t *instance);
extern void tinyrl_vt100_next_line(const tinyrl_vt100_t *instance);
extern void tinyrl_vt100_cursor_home(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_cursor_save(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_cursor_restore(const tinyrl_vt100_t * instance);
extern void tinyrl_vt100_erase(const tinyrl_vt100_t * instance, unsigned count);
extern void tinyrl_vt100_erase_down(const tinyrl_vt100_t * instance);
_END_C_DECL
#endif				/* _tinyrl_vt100_h */
/** @} tinyrl_vt100 */
