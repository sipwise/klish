/*
 * builtin_init.c
 */
#include <stdio.h>
#include <stdlib.h>

#include "clish/plugin.h"
#include "private.h"

/*----------------------------------------------------------- */
/* Initialize internal pseudo-plugin */
CLISH_PLUGIN_INIT(clish)
{
	/* Add hooks */
	clish_plugin_add_phook(plugin, clish_hook_access,
		"clish_hook_access", CLISH_SYM_TYPE_ACCESS);
	clish_plugin_add_phook(plugin, clish_hook_config,
		"clish_hook_config", CLISH_SYM_TYPE_CONFIG);
	clish_plugin_add_phook(plugin, clish_hook_log,
		"clish_hook_log", CLISH_SYM_TYPE_LOG);

	/* Add builtin syms */
	clish_plugin_add_psym(plugin, clish_close, "clish_close");
	clish_plugin_add_psym(plugin, clish_overview, "clish_overview");
	clish_plugin_add_psym(plugin, clish_source, "clish_source");
	clish_plugin_add_psym(plugin, clish_source_nostop, "clish_source_nostop");
	clish_plugin_add_psym(plugin, clish_history, "clish_history");
	clish_plugin_add_psym(plugin, clish_nested_up, "clish_nested_up");
	clish_plugin_add_psym(plugin, clish_nop, "clish_nop");
	clish_plugin_add_psym(plugin, clish_wdog, "clish_wdog");
	clish_plugin_add_psym(plugin, clish_macros, "clish_macros");
	clish_plugin_add_osym(plugin, clish_script, "clish_script");

	clish_shell = clish_shell; /* Happy compiler */

	return 0;
}

/*----------------------------------------------------------- */
