#ifdef DEBUG

/*
 * var_dump.c
 */

#include "lub/dump.h"
#include "clish/action.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_var_dump(const clish_var_t *this)
{
	lub_dump_printf("var(%p)\n", this);
	lub_dump_indent();

	lub_dump_printf("name     : %s\n", LUB_DUMP_STR(this->name));
	lub_dump_printf("dynamic  : %s\n", LUB_DUMP_BOOL(this->dynamic));
	lub_dump_printf("value    : %s\n", LUB_DUMP_STR(this->value));
	clish_action_dump(this->action);

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
