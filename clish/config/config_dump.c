#ifdef DEBUG

/*
 * config_dump.c
 */

#include "lub/dump.h"
#include "private.h"

/*--------------------------------------------------------- */
void clish_config_dump(const clish_config_t *this)
{
	char *op;

	lub_dump_printf("config(%p)\n", this);
	lub_dump_indent();

	switch (this->op) {
	case CLISH_CONFIG_NONE:
		op = "NONE";
		break;
	case CLISH_CONFIG_SET:
		op = "SET";
		break;
	case CLISH_CONFIG_UNSET:
		op = "UNSET";
		break;
	case CLISH_CONFIG_DUMP:
		op = "DUMP";
		break;
	default:
		op = "Unknown";
		break;
	}
	lub_dump_printf("op      : %s\n", op);

	lub_dump_undent();
}

/*--------------------------------------------------------- */

#endif /* DEBUG */
