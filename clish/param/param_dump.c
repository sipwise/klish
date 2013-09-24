/*
 * param_dump.c
 */
#include "private.h"
#include "lub/dump.h"
/*--------------------------------------------------------- */
void clish_param_dump(const clish_param_t * this)
{
	unsigned i;
	char *mode;

	lub_dump_printf("param(%p)\n", this);

	lub_dump_indent();
	lub_dump_printf("name       : %s\n", this->name);
	lub_dump_printf("text       : %s\n", this->text);
	lub_dump_printf("value      : %s\n", this->value);
	lub_dump_printf("ptype      : %s\n", clish_ptype__get_name(this->ptype));
	lub_dump_printf("default    : %s\n",
		this->defval ? this->defval : "(null)");
	switch (this->mode) {
	case CLISH_PARAM_COMMON:
		mode = "COMMON";
		break;
	case CLISH_PARAM_SWITCH:
		mode = "SWITCH";
		break;
	case CLISH_PARAM_SUBCOMMAND:
		mode = "SUBCOMMAND";
		break;
	default:
		mode = "Unknown";
		break;
	}
	lub_dump_printf("mode       : %s\n", mode);
	lub_dump_printf("paramc     : %d\n", clish_paramv__get_count(this->paramv));
	lub_dump_printf("optional   : %s\n",
		this->optional ? "true" : "false");
	lub_dump_printf("hidden     : %s\n",
		this->hidden ? "true" : "false");
	lub_dump_printf("test       : %s\n", this->test);
	lub_dump_printf("completion : %s\n", this->completion);

	/* Get each parameter to dump their details */
	for (i = 0; i < clish_paramv__get_count(this->paramv); i++) {
		clish_param_dump(clish_param__get_param(this, i));
	}

	lub_dump_undent();
}

/*--------------------------------------------------------- */
