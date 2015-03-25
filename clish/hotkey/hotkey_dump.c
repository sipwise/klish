#ifdef DEBUG

/*
 * hotkey_dump.c
 */
#include "private.h"
#include "lub/dump.h"

/*--------------------------------------------------------- */
void clish_hotkey_dump(const clish_hotkey_t *this)
{
	lub_dump_printf("hotkey(%p)\n", this);

	lub_dump_indent();
	lub_dump_printf("key : %d\n", this->code);
	lub_dump_printf("cmd : %s\n", LUB_DUMP_STR(this->cmd));
	lub_dump_undent();
}

/*--------------------------------------------------------- */
void clish_hotkeyv_dump(const clish_hotkeyv_t *this)
{
	unsigned int i;

	lub_dump_printf("hotkeyv(%p)\n", this);
	lub_dump_indent();
	for (i = 0; i < this->num; i++)
		clish_hotkey_dump(this->hotkeyv[i]);
	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
