/*
 * private.h
 *
 * Class to deal with splitting a command line into multiple arguments.
 * This class deals with full quoted text "like this" as a single argument.
 */
#include "lub/argv.h"

typedef struct lub_arg_s lub_arg_t;
struct lub_arg_s {
	char *arg;
	size_t offset;
	bool_t quoted;
};

struct lub_argv_s {
	unsigned argc;
	lub_arg_t *argv;
};
