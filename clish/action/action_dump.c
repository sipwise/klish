#ifdef DEBUG

/*
 * action_dump.c
 */

#include "lub/dump.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_action_dump(const clish_action_t *this)
{
	char *builtin_name;

	lub_dump_printf("action(%p)\n", this);
	lub_dump_indent();

	lub_dump_printf("script  : %s\n",
		this->script ? this->script : "(null)");
	builtin_name = clish_sym__get_name(this->builtin);
	lub_dump_printf("builtin : %s\n",
		builtin_name ? builtin_name : "(null)");
	lub_dump_printf("shebang : %s\n",
		this->shebang ? this->shebang : "(null)");

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
