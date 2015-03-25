#ifdef DEBUG

/*
 * action_dump.c
 */

#include "lub/dump.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_action_dump(const clish_action_t *this)
{
	lub_dump_printf("action(%p)\n", this);
	lub_dump_indent();

	lub_dump_printf("script  : %s\n", LUB_DUMP_STR(this->script));
	lub_dump_printf("builtin : %s\n",
		this->builtin ? clish_sym__get_name(this->builtin) : LUB_DUMP_NULL );
	lub_dump_printf("shebang : %s\n", LUB_DUMP_STR(this->shebang));

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
