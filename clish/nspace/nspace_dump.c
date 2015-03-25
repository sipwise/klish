#ifdef DEBUG

/*
 * nspace_dump.c
 */
#include "private.h"
#include "lub/dump.h"

/*--------------------------------------------------------- */
void clish_nspace_dump(const clish_nspace_t * this)
{
	lub_dump_printf("nspace(%p)\n", this);

	lub_dump_indent();
	lub_dump_printf("view         : %s\n",
		clish_view__get_name(this->view));
	lub_dump_printf("view_name    : %s\n", LUB_DUMP_STR(this->view_name));
	lub_dump_printf("prefix       : %s\n", LUB_DUMP_STR(this->prefix));
	lub_dump_printf("access       : %s\n", LUB_DUMP_STR(this->access));
	lub_dump_printf("help         : %s\n", LUB_DUMP_BOOL(this->help));
	lub_dump_printf("completion   : %s\n", LUB_DUMP_BOOL(this->completion));
	lub_dump_printf("context_help : %s\n", LUB_DUMP_BOOL(this->context_help));
	lub_dump_printf("inherit      : %s\n", LUB_DUMP_BOOL(this->inherit));
	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
