## Process this file with automake to generate Makefile.in
AUTOMAKE_OPTIONS = foreign nostdinc
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I. -I$(top_srcdir)
AM_LD = $(CC)
VERSION_INFO = -version-info $(SONAME_CURRENT):$(SONAME_REVISION):$(SONAME_AGE)
AM_LDFLAGS = $(VERSION_INFO)

if DEBUG
  DEBUG_CFLAGS = -DDEBUG
endif

AM_CFLAGS = -Wall $(DEBUG_CFLAGS)
#AM_CFLAGS = -ansi -pedantic -Werror -Wall -D_POSIX_C_SOURCE=199309 -DVERSION=$(VERSION) $(DEBUG_CFLAGS)

bin_PROGRAMS =
lib_LTLIBRARIES =
lib_LIBRARIES =
nobase_include_HEADERS =

EXTRA_DIST = \
	bin/module.am \
	clish/module.am \
	lub/module.am \
	tinyrl/module.am \
	konf/module.am \
	plugins/module.am \
	libc/module.am \
	doc/module.am \
	contrib \
	xml-examples \
	debian \
	clish.xsd \
	LICENCE \
	README \
	CHANGES \
	ISSUES

include $(top_srcdir)/lub/module.am
include $(top_srcdir)/tinyrl/module.am
include $(top_srcdir)/konf/module.am
include $(top_srcdir)/clish/module.am
include $(top_srcdir)/bin/module.am
include $(top_srcdir)/plugins/module.am
include $(top_srcdir)/libc/module.am
include $(top_srcdir)/doc/module.am
