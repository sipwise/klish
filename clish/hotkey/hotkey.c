/*
 * hotkey.c
 */
#include "private.h"
#include "lub/string.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Symbolic key codes */
const char *clish_hotkey_list[] = {
	"^@", /* 0 Null character */
	"^A", /* 1 Start of heading, = console interrupt */
	"^B", /* 2 Start of text, maintenance mode on HP console */
	"^C", /* 3 End of text */
	"^D", /* 4 End of transmission, not the same as ETB */
	"^E", /* 5 Enquiry, goes with ACK; old HP flow control */
	"^F", /* 6 Acknowledge, clears ENQ logon hand */
	"^G", /* 7 Bell, rings the bell... */
	"^H", /* 8 Backspace, works on HP terminals/computers */
	"^I", /* 9 Horizontal tab, move to next tab stop */
	"^J", /* 10 Line Feed */
	"^K", /* 11 Vertical tab */
	"^L", /* 12 Form Feed, page eject */
	"^M", /* 13 Carriage Return*/
	"^N", /* 14 Shift Out, alternate character set */
	"^O", /* 15 Shift In, resume defaultn character set */
	"^P", /* 16 Data link escape */
	"^Q", /* 17 XON, with XOFF to pause listings; "okay to send". */
	"^R", /* 18 Device control 2, block-mode flow control */
	"^S", /* 19 XOFF, with XON is TERM=18 flow control */
	"^T", /* 20 Device control 4 */
	"^U", /* 21 Negative acknowledge */
	"^V", /* 22 Synchronous idle */
	"^W", /* 23 End transmission block, not the same as EOT */
	"^X", /* 24 Cancel line, MPE echoes !!! */
	"^Y", /* 25 End of medium, Control-Y interrupt */
	"^Z", /* 26 Substitute */
	"^[", /* 27 Escape, next character is not echoed */
	"^\\", /* 28 File separator */
	"^]", /* 29 Group separator */
	"^^", /* 30 Record separator, block-mode terminator */
	"^_",  /* 31 Unit separator */
	NULL
	};

/*--------------------------------------------------------- */
/* Search for the specified hotkey and return its hotkey structure */
static clish_hotkey_t *find_hotkey(clish_hotkeyv_t *this, int code)
{
	unsigned int i;
	clish_hotkey_t *result = NULL;

	if (!this)
		return NULL;

	/* Scan the hotkey entries in this instance */
	for (i = 0; i < this->num; i++) {
		clish_hotkey_t *hk = this->hotkeyv[i];
		if (code == hk->code) {
			result = hk;
			break;
		}
	}
	return result;
}

/*--------------------------------------------------------- */
const char *clish_hotkeyv_cmd_by_code(clish_hotkeyv_t *this, int code)
{
	clish_hotkey_t *hk;
	if (!this)
		return NULL;
	hk = find_hotkey(this, code);
	if (!hk)
		return NULL;
	return hk->cmd;
}

/*--------------------------------------------------------- */
int clish_hotkeyv_insert(clish_hotkeyv_t *this,
	const char *key, const char *cmd)
{
	int code = -1;
	int i;
	if (!this)
		return -1;

	/* Find out key code */
	i = 0;
	while (clish_hotkey_list[i]) {
		if (!strcmp(clish_hotkey_list[i], key))
			code = i;
		i++;
	}
	if (code < 0)
		return -1;

	/* Search for existance of such hotkey */
	clish_hotkey_t *hk = find_hotkey(this, code);
	if (hk) {
		/* release the current value */
		lub_string_free(hk->cmd);
	} else {
		size_t new_size = ((this->num + 1) * sizeof(clish_hotkey_t *));
		clish_hotkey_t **tmp;
		/* resize the hotkeys vector */
		tmp = realloc(this->hotkeyv, new_size);
	
		this->hotkeyv = tmp;
		/* insert reference to the parameter */
		hk = malloc(sizeof(*hk));
		this->hotkeyv[this->num++] = hk;
		hk->code = code;
	}
	hk->cmd = NULL;
	if (cmd)
		hk->cmd = lub_string_dup(cmd);

	return 0;
}

/*--------------------------------------------------------- */
clish_hotkeyv_t *clish_hotkeyv_new(void)
{
	clish_hotkeyv_t *this;

	this = malloc(sizeof(clish_hotkeyv_t));
	this->num = 0;
	this->hotkeyv = NULL;

	return this;
}

/*--------------------------------------------------------- */
static void clish_hotkeyv_fini(clish_hotkeyv_t *this)
{
	unsigned int i;

	for (i = 0; i < this->num; i++) {
		lub_string_free(this->hotkeyv[i]->cmd);
		free(this->hotkeyv[i]);
	}
	free(this->hotkeyv);
}

/*--------------------------------------------------------- */
void clish_hotkeyv_delete(clish_hotkeyv_t *this)
{
	if (!this)
		return;

	clish_hotkeyv_fini(this);
	free(this);
}

/*--------------------------------------------------------- */
