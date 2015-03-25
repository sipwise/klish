/* It must be here to include config.h before another headers */
#include "lub/db.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#define DEFAULT_GETPW_R_SIZE_MAX 1024

struct passwd *lub_db_getpwnam(const char *name)
{
	long int size;
	char *buf;
	struct passwd *pwbuf; 
	struct passwd *pw = NULL;
	int res = 0;

#ifdef _SC_GETPW_R_SIZE_MAX
	if ((size = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	pwbuf = malloc(sizeof(*pwbuf) + size);
	if (!pwbuf)
		return NULL;
	buf = (char *)pwbuf + sizeof(*pwbuf);
	
	res = getpwnam_r(name, pwbuf, buf, size, &pw);

	if (res || !pw) {
		free(pwbuf);
		if (res != 0)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return pwbuf;
}

struct passwd *lub_db_getpwuid(uid_t uid)
{
	long int size;
	char *buf;
	struct passwd *pwbuf; 
	struct passwd *pw = NULL;
	int res = 0;

#ifdef _SC_GETPW_R_SIZE_MAX
	if ((size = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	pwbuf = malloc(sizeof(*pwbuf) + size);
	if (!pwbuf)
		return NULL;
	buf = (char *)pwbuf + sizeof(*pwbuf);
	
	res = getpwuid_r(uid, pwbuf, buf, size, &pw);

	if (NULL == pw) {
		free(pwbuf);
		if (res != 0)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return pwbuf;
}

struct group *lub_db_getgrnam(const char *name)
{
	long int size;
	char *buf;
	struct group *grbuf; 
	struct group *gr = NULL;
	int res = 0;

#ifdef _SC_GETGR_R_SIZE_MAX
	if ((size = sysconf(_SC_GETGR_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	grbuf = malloc(sizeof(*grbuf) + size);
	if (!grbuf)
		return NULL;
	buf = (char *)grbuf + sizeof(*grbuf);
	
	res = getgrnam_r(name, grbuf, buf, size, &gr);

	if (!gr) {
		free(grbuf);
		if (res != 0)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return grbuf;
}

struct group *lub_db_getgrgid(gid_t gid)
{
	long int size;
	char *buf;
	struct group *grbuf;
	struct group *gr = NULL;
	int res = 0;

#ifdef _SC_GETGR_R_SIZE_MAX
	if ((size = sysconf(_SC_GETGR_R_SIZE_MAX)) < 0)
		size = DEFAULT_GETPW_R_SIZE_MAX;
#else
	size = DEFAULT_GETPW_R_SIZE_MAX;
#endif
	grbuf = malloc(sizeof(struct group) + size);
	if (!grbuf)
		return NULL;
	buf = (char *)grbuf + sizeof(struct group);

	res = getgrgid_r(gid, grbuf, buf, size, &gr);

	if (!gr) {
		free(grbuf);
		if (res != 0)
			errno = res;
		else
			errno = ENOENT;
		return NULL;
	}

	return grbuf;
}
