#ifndef _lub_passwd_h
#define _lub_passwd_h

#include <stddef.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

/* Wrappers for ugly getpwnam_r()-like functions */
#ifdef HAVE_PWD_H
struct passwd *lub_db_getpwnam(const char *name);
struct passwd *lub_db_getpwuid(uid_t uid);
#endif
#ifdef HAVE_GRP_H
struct group *lub_db_getgrnam(const char *name);
struct group *lub_db_getgrgid(gid_t gid);
#endif

#endif

