/*
 * plugin.h
 */
#ifndef _clish_plugin_h
#define _clish_plugin_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "lub/types.h"

/* Symbol */

/* Symbol types. Functions with different purposes. */
typedef enum {
	CLISH_SYM_TYPE_NONE = 0, /* None */
	CLISH_SYM_TYPE_ACTION, /* Common builtin symbol for ACTION tag */
	CLISH_SYM_TYPE_ACCESS, /* Callback for "access" field */
	CLISH_SYM_TYPE_CONFIG, /* Callback for CONFIG tag */
	CLISH_SYM_TYPE_LOG, /* Callback for logging */
	CLISH_SYM_TYPE_MAX /* Number of elements */
} clish_sym_type_e;

/* External functions APIs. */
typedef enum {
	CLISH_SYM_API_SIMPLE = 0, /* Simple (may be single) API */
	CLISH_SYM_API_STDOUT /* Symbol for ACTION tag without "out" argument */
} clish_sym_api_e;

typedef struct clish_sym_s clish_sym_t;
typedef struct clish_plugin_s clish_plugin_t;

/* Plugin types */

/* Name of init function within plugin */
#define CLISH_PLUGIN_INIT_FNAME(name) clish_plugin_##name##_init
#define CLISH_PLUGIN_INIT_NAME_PREFIX "clish_plugin_"
#define CLISH_PLUGIN_INIT_NAME_SUFFIX "_init"
#define CLISH_PLUGIN_INIT_FUNC(name) int name(void *clish_shell, clish_plugin_t *plugin)
#define CLISH_PLUGIN_INIT(name) CLISH_PLUGIN_INIT_FUNC(CLISH_PLUGIN_INIT_FNAME(name))

#define CLISH_PLUGIN_FINI(name) int name(void *clish_shell, clish_plugin_t *plugin)
#define CLISH_PLUGIN_SYM(name) int name(void *clish_context, const char *script, char **out)
#define CLISH_PLUGIN_OSYM(name) int name(void *clish_context, const char *script)
#define CLISH_HOOK_ACCESS(name) int name(void *clish_shell, const char *access)
#define CLISH_HOOK_CONFIG(name) int name(void *clish_context)
#define CLISH_HOOK_LOG(name) int name(void *clish_context, const char *line, int retcode)

typedef CLISH_PLUGIN_INIT_FUNC(clish_plugin_init_t);
typedef CLISH_PLUGIN_FINI(clish_plugin_fini_t);
typedef CLISH_PLUGIN_SYM(clish_hook_action_fn_t);
typedef CLISH_PLUGIN_OSYM(clish_hook_oaction_fn_t);
typedef CLISH_HOOK_ACCESS(clish_hook_access_fn_t);
typedef CLISH_HOOK_CONFIG(clish_hook_config_fn_t);
typedef CLISH_HOOK_LOG(clish_hook_log_fn_t);

/* Helpers */
#define SYM_FN(TYPE,SYM) (*((clish_hook_##TYPE##_fn_t *)(clish_sym__get_func(SYM))))

/* Create an array of builtin plugin's init functions */
struct clish_plugin_builtin_list_s {
	const char *name; /* Plugin name */
	clish_plugin_init_t *init; /* Plugin init function */
};
typedef struct clish_plugin_builtin_list_s clish_plugin_builtin_list_t;
extern clish_plugin_builtin_list_t clish_plugin_builtin_list[];

/* Symbol */

int clish_sym_compare(const void *first, const void *second);
clish_sym_t *clish_sym_new(const char *name, void *func, int type);
void clish_sym_free(clish_sym_t *instance);
void clish_sym__set_func(clish_sym_t *instance, void *func);
void *clish_sym__get_func(clish_sym_t *instance);
void clish_sym__set_name(clish_sym_t *instance, const char *name);
char *clish_sym__get_name(clish_sym_t *instance);
void clish_sym__set_permanent(clish_sym_t *instance, bool_t permanent);
bool_t clish_sym__get_permanent(clish_sym_t *instance);
void clish_sym__set_plugin(clish_sym_t *instance, clish_plugin_t *plugin);
clish_plugin_t *clish_sym__get_plugin(clish_sym_t *instance);
void clish_sym__set_type(clish_sym_t *instance, int type);
int clish_sym__get_type(const clish_sym_t *instance);
void clish_sym__set_api(clish_sym_t *instance, clish_sym_api_e api);
clish_sym_api_e clish_sym__get_api(const clish_sym_t *instance);
int clish_sym_clone(clish_sym_t *dst, clish_sym_t *src);

/* Plugin */

clish_plugin_t *clish_plugin_new(const char *name);
void clish_plugin_free(clish_plugin_t *instance, void *userdata);
int clish_plugin_load(clish_plugin_t *instance, void *userdata);
clish_sym_t *clish_plugin_get_sym(clish_plugin_t *instance,
	const char *name, int type);
clish_sym_t *clish_plugin_add_generic(clish_plugin_t *instance,
	void *func, const char *name, int type, bool_t permanent);
clish_sym_t *clish_plugin_add_sym(clish_plugin_t *instance,
	clish_hook_action_fn_t *func, const char *name);
clish_sym_t *clish_plugin_add_psym(clish_plugin_t *instance,
	clish_hook_action_fn_t *func, const char *name);
clish_sym_t *clish_plugin_add_osym(clish_plugin_t *instance,
	clish_hook_oaction_fn_t *func, const char *name);
clish_sym_t *clish_plugin_add_posym(clish_plugin_t *instance,
	clish_hook_oaction_fn_t *func, const char *name);
clish_sym_t *clish_plugin_add_hook(clish_plugin_t *instance,
	void *func, const char *name, int type);
clish_sym_t *clish_plugin_add_phook(clish_plugin_t *instance,
	void *func, const char *name, int type);
void clish_plugin_add_fini(clish_plugin_t *instance,
	clish_plugin_fini_t *fini);
clish_plugin_fini_t * clish_plugin_get_fini(clish_plugin_t *instance);
void clish_plugin_add_init(clish_plugin_t *instance,
	clish_plugin_init_t *init);
clish_plugin_init_t * clish_plugin_get_init(clish_plugin_t *instance);
void clish_plugin_dump(const clish_plugin_t *instance);
char *clish_plugin__get_name(const clish_plugin_t *instance);
void clish_plugin__set_alias(clish_plugin_t *instance, const char *alias);
char *clish_plugin__get_alias(const clish_plugin_t *instance);
char *clish_plugin__get_pubname(const clish_plugin_t *instance);
void clish_plugin__set_file(clish_plugin_t *instance, const char *file);
char *clish_plugin__get_file(const clish_plugin_t *instance);
void clish_plugin__set_builtin_flag(clish_plugin_t *instance, bool_t builtin_flag);
bool_t clish_plugin__get_builtin_flag(const clish_plugin_t *instance);
void clish_plugin__set_conf(clish_plugin_t *instance, const char *conf);
char *clish_plugin__get_conf(const clish_plugin_t *instance);
void clish_plugin__set_rtld_global(clish_plugin_t *instance, bool_t rtld_global);
bool_t clish_plugin__get_rtld_global(const clish_plugin_t *instance);

#endif				/* _clish_plugin_h */
/** @} clish_plugin */
