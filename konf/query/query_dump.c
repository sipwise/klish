#ifdef DEBUG

#include "private.h"
#include "lub/dump.h"

/*-------------------------------------------------------- */
void konf_query_dump(konf_query_t *this)
{
	const char *op;

	lub_dump_printf("query(%p)\n", this);
	lub_dump_indent();

	switch (this->op) {
	case KONF_QUERY_OP_SET:
		op = "SET";
		break;
	case KONF_QUERY_OP_UNSET:
		op = "UNSET";
		break;
	case KONF_QUERY_OP_DUMP:
		op = "DUMP";
		break;
	case KONF_QUERY_OP_OK:
		op = "OK";
		break;
	case KONF_QUERY_OP_ERROR:
		op = "ERROR";
		break;
	case KONF_QUERY_OP_STREAM:
		op = "STREAM";
		break;
	default:
		op = "UNKNOWN";
		break;
	}
	lub_dump_printf("operation : %s\n", op);
	lub_dump_printf("pattern   : %s\n", this->pattern);
	lub_dump_printf("priority  : 0x%x\n", this->priority);
	lub_dump_printf("sequence  : %u\n", this->seq ? "true" : "false");
	lub_dump_printf("seq_num   : %u\n", this->seq_num);
	lub_dump_printf("line      : %s\n", this->line);
	lub_dump_printf("path      : %s\n", this->path);
	lub_dump_printf("pwdc      : %u\n", this->pwdc);
	lub_dump_printf("splitter  : %s\n", this->splitter ? "true" : "false");
	lub_dump_printf("unique    : %s\n", this->unique ? "true" : "false");
	lub_dump_printf("depth     : %d\n", this->depth);

	lub_dump_undent();
}

#endif /* DEBUG */
