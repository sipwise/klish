/*
 * lub/ini/private.h
 *
 * Class to parse ini-like strings/files.
 */

#include "lub/string.h"
#include "lub/list.h"
#include "lub/ini.h"

struct lub_pair_s {
	char *name;
	char *value;
};

struct lub_ini_s {
	lub_list_t *list;
};
