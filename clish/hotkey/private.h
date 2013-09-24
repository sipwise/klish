/*
 * hotkey private.h
 */

#include "clish/hotkey.h"

/*---------------------------------------------------------
 * PRIVATE TYPES
 *--------------------------------------------------------- */

struct clish_hotkey_s {
	int code; /* Hotkey code */
	char *cmd; /* Command to execute on this hotkey */
};

struct clish_hotkeyv_s {
	unsigned int num;
	clish_hotkey_t **hotkeyv;
};
