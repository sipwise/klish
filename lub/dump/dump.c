/*
 * dump.c
 * Provides indented printf functionality
 */
#include "private.h"

#include <stdio.h>
#include <stdarg.h>

static int indent = 0;

/*--------------------------------------------------------- */
int lub_dump_printf(const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	fprintf(stderr, "%*s", indent, "");
	len = vfprintf(stderr, fmt, args);
	va_end(args);

	return len;
}

/*--------------------------------------------------------- */
static void lub_dump_divider(char c)
{
	int i;

	lub_dump_printf("");
	for (i = 0; i < (80 - indent); i++) {
		fputc(c, stderr);
	}
	fputc('\n', stderr);
}

/*--------------------------------------------------------- */
void lub_dump_indent(void)
{
	indent += 2;
	lub_dump_divider('_');
}

/*--------------------------------------------------------- */
void lub_dump_undent(void)
{
	lub_dump_divider('^');

	indent -= 2;
}

/*--------------------------------------------------------- */
