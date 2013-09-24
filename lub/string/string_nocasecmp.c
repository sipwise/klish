/*
 * string_nocasecmp.c
 */
#include <string.h>
#include <ctype.h>

#include "private.h"
#include "lub/ctype.h"


/*--------------------------------------------------------- */
int lub_string_nocasecmp(const char *cs, const char *ct)
{
	int result = 0;
	while ((0 == result) && *cs && *ct) {
		/*lint -e155 Ignoring { }'ed sequence within an expression, 0 assumed 
		 * MACRO implementation uses braces to prevent multiple increments
		 * when called.
		 */
		int s = lub_ctype_tolower(*cs++);
		int t = lub_ctype_tolower(*ct++);

		result = s - t;
	}
	/*lint -e774 Boolean within 'if' always evealuates to True 
	 * not the case because of tolower() evaluating to 0 under lint
	 * (see above)
	 */
	if (0 == result) {
		/* account for different string lengths */
		result = *cs - *ct;
	}
	return result;
}

/*--------------------------------------------------------- */
char *lub_string_tolower(const char *str)
{
	char *tmp = strdup(str);
	char *p = tmp;

	while (*p) {
		*p = tolower(*p);
		p++;
	}

	return tmp;
}


/*--------------------------------------------------------- */
