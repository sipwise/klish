#ifdef DEBUG

/*
 * pargv_dump.c
 */
#include "private.h"
#include "lub/dump.h"

/*--------------------------------------------------------- */
void clish_parg_dump(const clish_parg_t * this)
{
	lub_dump_printf("parg(%p)\n", this);
	lub_dump_indent();
	lub_dump_printf("name : %s\n", clish_parg__get_name(this));
	lub_dump_printf("ptype: %s\n",
		clish_ptype__get_name(clish_parg__get_ptype(this)));
	lub_dump_printf("value: %s\n", clish_parg__get_value(this));
	lub_dump_undent();
}

/*--------------------------------------------------------- */
void clish_pargv_dump(const clish_pargv_t * this)
{
	unsigned i;

	lub_dump_printf("pargv(%p)\n", this);
	lub_dump_indent();
	for (i = 0; i < this->pargc; i++) {
		/* get the appropriate parameter definition */
		clish_parg_dump(this->pargv[i]);
	}
	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
