#ifndef _lub_log_h
#define _lub_log_h

#include <syslog.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

int lub_log_facility(const char *str, int *facility);

#endif

