#include "tinyrl/vt100.h"

struct _tinyrl_vt100 {
	FILE *istream;
	FILE *ostream;
	int   timeout; /* Input timeout in seconds */
};
