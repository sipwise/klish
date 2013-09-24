/*
 * hotkey.h
 */
#ifndef _clish_hotkey_h
#define _clish_hotkey_h

typedef struct clish_hotkey_s clish_hotkey_t;
typedef struct clish_hotkeyv_s clish_hotkeyv_t;

const char *clish_hotkeyv_cmd_by_code(clish_hotkeyv_t *instance, int code);
int clish_hotkeyv_insert(clish_hotkeyv_t *instance,
	const char *key, const char *cmd);
clish_hotkeyv_t *clish_hotkeyv_new(void);
void clish_hotkeyv_delete(clish_hotkeyv_t *instance);

#endif				/* _clish_hotkey_h */
/** @} clish_hotkey */
