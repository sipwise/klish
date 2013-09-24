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

	lub_dump_printf("script  : %s\n",
		this->script ? this->script : "(null)");
	lub_dump_printf("builtin : %s\n",
		this->builtin ? this->builtin : "(null)");
	lub_dump_printf("shebang : %s\n",
		this->shebang ? this->shebang : "(null)");

	lub_dump_undent();
}

/*--------------------------------------------------------- */
