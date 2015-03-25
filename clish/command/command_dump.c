#ifdef DEBUG

/*
 * command_dump.c
 */

#include "lub/dump.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_command_dump(const clish_command_t * this)
{
	unsigned i;

	lub_dump_printf("command(%p)\n", this);
	lub_dump_indent();
	lub_dump_printf("name       : %s\n", this->name);
	lub_dump_printf("text       : %s\n", this->text);
	lub_dump_printf("link       : %s\n",
		this->link ?
		clish_command__get_name(this->link) : LUB_DUMP_NULL);
	lub_dump_printf("alias      : %s\n", LUB_DUMP_STR(this->alias));
	lub_dump_printf("alias_view : %s\n", LUB_DUMP_STR(this->alias_view));
	lub_dump_printf("paramc     : %d\n",
		clish_paramv__get_count(this->paramv));
	lub_dump_printf("detail     : %s\n", LUB_DUMP_STR(this->detail));
	lub_dump_printf("access     : %s\n", LUB_DUMP_STR(this->access));
	clish_action_dump(this->action);
	clish_config_dump(this->config);

	/* Get each parameter to dump their details */
	for (i = 0; i < clish_paramv__get_count(this->paramv); i++) {
		clish_param_dump(clish_command__get_param(this, i));
	}

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
