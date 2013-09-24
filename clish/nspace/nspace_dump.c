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
	lub_dump_printf("prefix       : %s\n",
			this->prefix ? this->prefix : "(null)");
	lub_dump_printf("help         : %s\n", this->help ? "true" : "false");
	lub_dump_printf("completion   : %s\n",
			this->completion ? "true" : "false");
	lub_dump_printf("context_help : %s\n",
			this->context_help ? "true" : "false");
	lub_dump_printf("inherit      : %s\n",
			this->inherit ? "true" : "false");
	lub_dump_undent();
}

/*--------------------------------------------------------- */
