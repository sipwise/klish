/* private.h */
#include "tinyrl/history.h"
/**************************************
 * protected interface to tinyrl_history_entry class
 ************************************** */
extern tinyrl_history_entry_t *tinyrl_history_entry_new(const char *line,
							unsigned index);

extern void tinyrl_history_entry_delete(tinyrl_history_entry_t * instance);
