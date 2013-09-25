/*
 * udata.h - private interface to the userdata class
 */

#include "clish/udata.h"

struct clish_udata_s {
	char *name;
	void *data;
};
