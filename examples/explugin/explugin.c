#include <stdlib.h>
#include <stdio.h>
#include <clish/plugin.h>

CLISH_PLUGIN_SYM(explugin_fn)
{
	printf("explugin: Hello world\n");
	return 0;
}

CLISH_PLUGIN_INIT
{
	clish_plugin_add_sym(plugin, explugin_fn, "hello");

	return 0;
}


