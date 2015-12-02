#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 2)
m4_define([MINOR_VERSION], 1)
m4_define([MICRO_VERSION], 1)

AC_PREREQ(2.59)
AC_INIT([klish],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)
AC_CONFIG_MACRO_DIR([m4])

# Values for SONAME. See -version-info for details.
AC_SUBST(SONAME_CURRENT, 1)
AC_SUBST(SONAME_REVISION, 0)
AC_SUBST(SONAME_AGE, 0)

# Check for system extensions (_POSIX_THREAD_SEMANTICS for Solaris)
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(subdir-objects)
AM_PROG_CC_C_O

# Dir for libc replacements
AC_CONFIG_LIBOBJ_DIR([libc])

# needed to handle 64-bit architecture
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(size_t)

#########################################
# See if linker supports version scripts
#########################################
# Check if LD supports linker scripts,
# and define automake conditional HAVE_LD_VERSION_SCRIPT if so.
AC_ARG_ENABLE([ld-version-script],
              AS_HELP_STRING([--enable-ld-version-script],
                             [enable linker version script (default is enabled when possible)]),
                             [have_ld_version_script=$enableval], [])
if test -z "$have_ld_version_script"; then
    AC_MSG_CHECKING([if LD -Wl,--version-script works])
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,--version-script=conftest.map"
    cat > conftest.map <<EOF
VERS_1 {
    global: sym;
};

VERS_2 {
    global: sym;
} VERS_1;
EOF
    AC_LINK_IFELSE([AC_LANG_SOURCE([int main() { return 0; }])],
                   [have_ld_version_script=yes], [have_ld_version_script=no])
    rm -f conftest.map
    LDFLAGS="$save_LDFLAGS"
    AC_MSG_RESULT($have_ld_version_script)
fi
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "$have_ld_version_script" = "yes")

################################
# Deal with debugging options
################################
AC_ARG_ENABLE(debug,
              [AS_HELP_STRING([--enable-debug],
                              [Turn on debugging including asserts [default=no]])],
              [],
              [enable_debug=no])
AM_CONDITIONAL(DEBUG,test x$enable_debug = xyes)

################################
# Check for Lua support
################################
AC_ARG_WITH(lua,
            [AS_HELP_STRING([--with-lua=DIR],
                            [Build Lua ACTION plugin [default=no]])],
            [use_lua=$withval],
            [use_lua=no])
AM_CONDITIONAL(WITH_LUA,test x$use_lua != xno)

if test x$use_lua != xno; then
    if test x$use_lua != xyes; then
        CPPFLAGS="${CPPFLAGS} -I$use_lua/include"
	LDFLAGS="${LDFLAGS} -L$use_lua/lib"
    fi
    LUA_VERSION="5.1"
    AX_LUA_HEADERS()
    AX_LUA_LIBS()
fi

################################
# Check for the roxml library
################################
AC_ARG_WITH(libroxml,
            [AS_HELP_STRING([--with-libroxml=DIR],
                            [Use roxml as the XML parser implementation [default=no]])],
            [use_roxml=$withval],
            [use_roxml=no])

AC_ARG_WITH(libexpat,
            [AS_HELP_STRING([--with-libexpat=DIR],
                            [Use expat as the XML parser implementation [default=no]])],
            [use_expat=$withval],
            [use_expat=no])

AC_ARG_WITH(libxml2,
            [AS_HELP_STRING([--with-libxml2=DIR],
                            [Use libxml2 as the XML parser implementation [default=no]])],
            [use_libxml2=$withval],
            [use_libxml2=no])

# select the default xml backend
sel_xml_backends=""
xml_backend=""
found_xml_backend=""
count_xml_backends=0
if test "x$use_libxml2" != "xno"; then
    sel_xml_backends="$sel_xml_backends libxml2"
    xml_backend="libxml2"
    count_xml_backends=$((count_xml_backends + 1))
fi

if test "x$use_roxml" != "xno"; then
    sel_xml_backends="$sel_xml_backends roxml"
    xml_backend="roxml"
    count_xml_backends=$((count_xml_backends + 1))
fi

if test "x$use_expat" != "xno"; then
    sel_xml_backends="$sel_xml_backends expat"
    xml_backend="expat"
    count_xml_backends=$((count_xml_backends + 1))
fi

if test $count_xml_backends -gt 1; then
    AC_MSG_WARN([Multiple XML backend has been selected ($sel_xml_backends). I choose $xml_backend])
fi

if test "x$xml_backend" = "x"; then
    xml_backend="auto"
    AC_MSG_WARN([No XML backend has been selected: auto check])
fi

case x$xml_backend in
    xroxml)
        use_libxml2="no"
        use_expat="no"
        ;;
    xlibxml2)
        use_roxml="no"
        use_expat="no"
        ;;
    xexpat)
        use_libxml2="no"
        use_roxml="no"
        ;;
esac

XML_LDFLAGS=""
XML_CFLAGS=""
XML_LIBS=""

if test "$xml_backend" = "expat" -o "$xml_backend" = "auto"; then
    if test "$xml_backend" = "auto"; then
        # on auto select, we try to detect the library
        use_expat="yes"
    fi
    case x$use_expat in
      xyes)
        # we choose to NOT rely on pkg-config on this one. Instead, we
        # check for the library and the header file - that should be
        # enough.
        AC_CHECK_HEADER([expat.h],
                        [expat_h_found=yes],
                        [expat_h_found=no],
                        [/* force include check */])
        if test "x$expat_h_found" != "xyes"; then
            AC_CHECK_HEADER([bsdxml.h],
                            [expat_h_found=yes],
                            [expat_h_found=no],
                            [/* force include check */])
            if test "x$expat_h_found" != "xyes"; then
                if test "$xml_backend" = "auto"; then
                    AC_MSG_WARN([cannot find <expat.h> header file])
                else
                    AC_MSG_ERROR([cannot find <expat.h> header file])
                fi
            fi
        fi
        XML_CFLAGS=""

        AC_CHECK_LIB([expat],
                     [XML_ParserCreate],
                     [expat_lib_found=yes],
                     [expat_lib_found=no],
                     [])
        if test "x$expat_lib_found" != "xyes"; then
            AC_CHECK_LIB([bsdxml],
                         [XML_ParserCreate],
                         [expat_lib_found=yes],
                         [expat_lib_found=no],
                         [])
            if test "x$expat_lib_found" != "xno"; then
                XML_LIBS="-lbsdxml"
                AC_DEFINE([HAVE_LIB_BSDXML],
                          [],
                          [libbsdxml-based XML backend])
            else
                if test "$xml_backend" = "auto"; then
                    AC_MSG_WARN([cannot find expat library])
                else
                    AC_MSG_ERROR([cannot find expat library])
                fi
            fi
        else
            XML_LIBS="-lexpat"
        fi
        XML_LDFLAGS=""

        AC_DEFINE([HAVE_LIB_EXPAT],
                  [],
                  [libexpat-based XML backend])
        xml_backend="found"
        found_xml_backend="expat"
        ;;
      *)
        # this is probably broken. We consider that the user supplied path is
        # a non-standard path. But we're not going to check anything.
        AC_MSG_WARN([--with-expat=DIR is probably broken, just trying])
        XML_LDFLAGS="-L${use_expat}/lib"
        XML_CFLAGS="-I${use_expat}/include"
        XML_LIBS="-lexpat"
        AC_MSG_CHECKING([for expat support])
        AC_MSG_RESULT([yes])
        AC_MSG_NOTICE([headers for expat hopefully in ${use_expat}/include])
        AC_MSG_NOTICE([library expat hopefully in ${use_expat}/lib])
        AC_DEFINE([HAVE_LIB_EXPAT],
                  [],
                  [expat-based XML backend])
        xml_backend="found"
        found_xml_backend="expat"
        ;;
    esac
else
    AC_MSG_CHECKING([for libexpat support])
    AC_MSG_RESULT([no])
fi

if test "$xml_backend" = "roxml" -o "$xml_backend" = "auto"; then
    if test "$xml_backend" = "auto"; then
        # on auto select, we try to detect the library
        use_roxml="yes"
    fi
    case x$use_roxml in
      xyes)
        # we choose to NOT rely on pkg-config on this one. We may do it as
        # libroxml provides a .pc file but some environment (both cross-compile
        # or native environment) may lack this support. The good thing is that
        # it doesn't add much complexity to the configure.ac file (and we
        # may move these tests to another m4 file later).

        # the header is installed in the standard path
        AC_CHECK_HEADER([roxml.h],
                        [roxml_h_found=yes],
                        [roxml_h_found=no],
                        [/* force include check */])
        if test "x$roxml_h_found" != "xyes"; then
            if test "$xml_backend" = "auto"; then
                AC_MSG_WARN([cannot find <roxml.h> header file])
            else
                AC_MSG_ERROR([cannot find <roxml.h> header file])
            fi
        fi
        XML_CFLAGS=""

        # the library is installed in the standard path
        AC_CHECK_LIB([roxml],
                     [roxml_load_doc],
                     [roxml_lib_found=yes],
                     [roxml_lib_found=no],
                     [])
        if test "x$roxml_lib_found" != "xyes"; then
            if test "$xml_backend" = "auto"; then
                AC_MSG_WARN([cannot find roxml library])
            else
                AC_MSG_ERROR([cannot find roxml library])
            fi
        fi
        XML_LDFLAGS=""
        XML_LIBS="-lroxml"

        AC_DEFINE([HAVE_LIB_ROXML],
                  [],
                  [libroxml-based XML backend])
        xml_backend="found"
        found_xml_backend="roxml"
        ;;

      *)
        # first, we check if we're not looking for an alternate include
        # directory -for example, if the user feeds the script with the
        # option --with-roxml=/usr/local
        # NOTE: we search for include/roxml.h and inc/roxml.h to defeat
        # the caching algorithm of the configure script. If someone knows
        # a better way, please do not hesitate
        roxml_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS -I${use_roxml}"
        AC_CHECK_HEADER([include/roxml.h],
                        [roxml_h_found=yes],
                        [roxml_h_found=no],
                        [/* force include check */])
        if test "x$roxml_h_found" = "xno"; then
            # the directory might be a source directory, so check
            # if the include file is to be found here
            AC_CHECK_HEADER([inc/roxml.h],
                            [roxml_h_found=yes],
                            [roxml_h_found=no],
                            [/* force include check */])
            if test "x$roxml_h_found" = "xno"; then
                if test "$xml_backend" = "auto"; then
                    AC_MSG_WARN([cannot find <roxml.h> header file])
                else
                    AC_MSG_ERROR([cannot find <roxml.h> header file])
                fi
            fi
            XML_CFLAGS="-I${use_roxml}/inc"
            AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/inc])
        else
            XML_CFLAGS="-I${use_roxml}/include"
            AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/include])
        fi
        CFLAGS="$roxml_CFLAGS"

        # we're doing both previous checks, but we are trying to find a library
        # now, so the check themselves are a bit different
        # NOTE: we search for roxml_load_doc and roxml_close to defeat
        # the caching algorithm of the configure script. If someone knows
        # a better way, please do not hesitate.
        roxml_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS -L${use_roxml}/lib"
        AC_CHECK_LIB([roxml],
                     [roxml_load_doc],
                     [roxml_lib_found=yes],
                     [roxml_lib_found=no],
                     [])
        LDFLAGS=$roxml_LDFLAGS
        if test "x$roxml_lib_found" = "xno"; then
            LDFLAGS="$LDFLAGS -L${use_roxml}"
            AC_CHECK_LIB([roxml],
                         [roxml_close],
                         [roxml_lib_found=yes],
                         [roxml_lib_found=no],
                         [])
            LDFLAGS=$roxml_LDFLAGS
            if test "x$roxml_lib_found" = "xno"; then
                if test "$xml_backend" = "auto"; then
                    AC_MSG_WARN([cannot find roxml library])
                else
                    AC_MSG_ERROR([cannot find roxml library])
                fi
            fi
            XML_LDFLAGS="-L${use_roxml}"
            XML_LIBS="-lroxml"
            AC_MSG_NOTICE([library libroxml found in ${use_roxml}])
        else
            XML_LDFLAGS="-L${use_roxml}/lib"
            XML_LIBS="-lroxml"
            AC_MSG_NOTICE([library libroxml found in ${use_roxml}/lib])
        fi

        AC_DEFINE([HAVE_LIB_ROXML],
                  [],
                  [libroxml-based XML backend])
        xml_backend="found"
        found_xml_backend="roxml"
        ;;
    esac
else
    AC_MSG_CHECKING([for libroxml support])
    AC_MSG_RESULT([no])
fi

if test "$xml_backend" = "libxml2" -o "$xml_backend" = "auto"; then
    if test "$xml_backend" = "auto"; then
        # on auto select, we try to detect the library
        use_libxml2="yes"
    fi
    case x$use_libxml2 in
      xyes)
        # I would love to avoid using pkg-config (which may not be available on
        # some compilation environment) but doing so really add a lot of
        # complexity to the system, as the headers don't lie in a standard
        # directory (they lie in a subdirectory of a standard include directory;
        # not the same thing for configure scripts).
        XML_CFLAGS="`pkg-config libxml-2.0 --cflags`"
        XML_LDFLAGS="`pkg-config libxml-2.0 --libs-only-L`"
        XML_LIBS="`pkg-config libxml-2.0 --libs-only-l`"
        AC_CHECK_LIB([xml2],
                     [xmlNewDoc],
                     [libxml2_lib_found=yes],
                     [libxml2_lib_found=no],
                     [])
        if test "x$libxml2_lib_found" != "xyes"; then
            if test "$xml_backend" = "auto"; then
                AC_MSG_WARN([cannot find libxml2 library])
            else
                AC_MSG_ERROR([cannot find libxml2 library])
            fi
        fi

        # the header file is installed in a subdirectory of one of the standard
        # include directory. This might prove to be a problem if the cross-
        # compile environment is not complete enough (i.e. if it misses
        # pkg-config, or if pkg-config returns wrong values). In most cases, the
        # environment is likely to be OK so we will never hit any issue.
        xml2_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $XML_CFLAGS"
        AC_CHECK_HEADER([libxml/tree.h],
                        [libxml2_h_found=yes],
                        [libxml2_h_found=no],
                        [/* force include check */])
        CFLAGS="$xml2_CFLAGS"
        if test "x$libxml2_h_found" != "xyes"; then
            if test "$xml_backend" = "auto"; then
                AC_MSG_WARN([cannot find libxml2 headers])
            else
                AC_MSG_ERROR([cannot find libxml2 headers])
            fi
        fi

        AC_DEFINE([HAVE_LIB_LIBXML2],
                  [],
                  [libxml2-based XML backend])
        xml_backend="found"
        found_xml_backend="libxml2"
        ;;
      *)
        # this is probably broken. We consider that the user supplied path is
        # a non-standard path. But we're not going to check anything.
        AC_MSG_WARN([--with-libxml2=DIR is probably broken, just trying])
        XML_LDFLAGS="-L${use_libxml2}/lib"
        XML_CFLAGS="-I${use_libxml2}/include/libxml2"
        XML_LIBS="-lxml2"
        AC_MSG_CHECKING([for libxml2 support])
        AC_MSG_RESULT([yes])
        AC_MSG_NOTICE([headers for libxml2 hopefully in ${use_libxml2}/include/libxml2])
        AC_MSG_NOTICE([library libxml2 hopefully in ${use_libxml2}/lib])
        AC_DEFINE([HAVE_LIB_LIBXML2],
                  [],
                  [libxml2-based XML backend])
        xml_backend="found"
        found_xml_backend="libxml2"
        ;;
    esac
else
    # not selected? We print a small message
    AC_MSG_CHECKING([for libxml2 support])
    AC_MSG_RESULT([no])
fi

if test "$xml_backend" != "found"; then
    AC_MSG_ERROR([Failed to find a suitable XML backend])
fi

if test $count_xml_backends -eq 0; then
    AC_MSG_NOTICE([I found a suitable XML backend: $found_xml_backend])
fi

# LIBXSLT
AC_ARG_WITH(libxslt,
            [AS_HELP_STRING([--with-libxslt=DIR],
                            [Use libxslt as the XSLT transform engine [default=no]. Depend on libxml2 library.])],
            [use_libxslt=$withval],
            [use_libxslt=no])

if test "x$use_libxslt" != "xno" -a "$found_xml_backend" != "libxml2"; then
    AC_MSG_ERROR([The libxml2 is necessary for libxslt])
fi

################################
# Check for the libxslt transform engine
################################

if test x$use_libxslt != xno; then
	AC_CHECK_LIB([xslt], [xsltApplyStylesheet], [libxslt_lib_found=yes], [libxslt_lib_found=no])
	if test "x$libxslt_lib_found" != "xyes"; then
		AC_MSG_ERROR([Can't find an XSLT library])
	fi
	XSLT_CFLAGS="`pkg-config libxslt --cflags 2>/dev/null`"
	XSLT_LDFLAGS="`pkg-config libxslt --libs-only-L 2>/dev/null`"
	XSLT_LIBS="`pkg-config libxslt --libs-only-l 2>/dev/null`"
	if test "x${XSLT_LIBS}" = "x"; then
		XSLT_LIBS="-lxslt"
	fi
	XML_CFLAGS="${XML_CFLAGS} ${XSLT_CFLAGS}"
	XML_LDFLAGS="${XML_LDFLAGS} ${XSLT_LDFLAGS}"
	XML_LIBS="${XML_LIBS} ${XSLT_LIBS}"
        AC_DEFINE([HAVE_LIB_LIBXSLT], [], [libxslt XML transform engine])
fi

################################
# Common XML related subst
################################

AC_SUBST(XML_LIBS)
AC_SUBST(XML_LDFLAGS)
AC_SUBST(XML_CFLAGS)

################################
# Search for network functions (like connect())
################################
AC_SEARCH_LIBS([socket], [socket])

################################
# Check for regex.h
################################
AC_CHECK_HEADERS(regex.h, [],
    AC_MSG_ERROR([regex.h not found: regular expressions are not supported]))

################################
# Internal getopt()
################################
AC_ARG_WITH(internal-getopt,
            [AS_HELP_STRING([--with-internal-getopt],
            [Use internal implementation of getopt [default=no]])],
            [],
            [with_internal_getopt=no])

if test x$with_internal_getopt != xno; then
    AC_DEFINE([WITH_INTERNAL_GETOPT], [1], [Use internal getopt() implementation])
    AC_LIBOBJ([getopt])
    AC_MSG_WARN([Use internal implementation of getopt() and getopt_long()])
else
    AC_CHECK_HEADERS(getopt.h, [found_getopt_h=yes],
        AC_MSG_WARN([getopt.h not found: only short parameters can be used on command line]))
fi

AC_MSG_CHECKING([for getopt_long()])
if test x$with_internal_getopt = xyes -o x$found_getopt_h = xyes; then
    AC_DEFINE([HAVE_GETOPT_LONG], [1], [getopt_long() function])
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

################################
# Check for locale.h
################################
AC_CHECK_HEADERS(locale.h, [],
    AC_MSG_WARN([locale.h not found: the locales is not supported]))

################################
# Check for CODESET within nl_langinfo
################################
AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], [am_cv_langinfo_codeset],
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET); return !cs;],
      [am_cv_langinfo_codeset=yes],
      [am_cv_langinfo_codeset=no])
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE([HAVE_LANGINFO_CODESET], [1],
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])
AM_LANGINFO_CODESET

################################
# Check for pwd.h and grp.h
################################
AC_CHECK_HEADERS(pwd.h, [],
    AC_MSG_WARN([pwd.h not found: the pwd operations is not supported]))
AC_CHECK_HEADERS(grp.h, [],
    AC_MSG_WARN([grp.h not found: the grp operations is not supported]))

################################
# Check for chroot
################################
AC_CHECK_FUNCS(chroot, [],
    AC_MSG_WARN([chroot() not found: the choot is not supported]))

################################
# Check for dlopen
################################
CLISH_PLUGIN_BUILTIN_LIST=
CLISH_PLUGIN_BUILTIN_DEFS=
CLISH_PLUGIN_BUILTIN_LIBS=

AC_DEFUN([AC_PLUGIN_BUILTIN],
[
  CLISH_PLUGIN_BUILTIN_LIBS="$CLISH_PLUGIN_BUILTIN_LIBS clish_plugin_$1.la"
  CLISH_PLUGIN_BUILTIN_DEFS="$CLISH_PLUGIN_BUILTIN_DEFS CLISH_PLUGIN_INIT($1);"
  CLISH_PLUGIN_BUILTIN_LIST="$CLISH_PLUGIN_BUILTIN_LIST { \"$1\", clish_plugin_$1_init },"
])

AC_CHECK_HEADERS(dlfcn.h, [
        AC_SEARCH_LIBS([dlopen], [dl dld], [], [
          AC_MSG_ERROR([unable to find the dlopen() function])
        ])
    ],[
          AC_MSG_WARN([dlfcn.h not found: the dl operations is not supported])
          AC_PLUGIN_BUILTIN([clish])
          if test x$use_lua != xno; then
            AC_PLUGIN_BUILTIN([lua])
          fi
    ])

AC_SUBST([CLISH_PLUGIN_BUILTIN_LIST])
AC_SUBST([CLISH_PLUGIN_BUILTIN_DEFS])
AC_SUBST([CLISH_PLUGIN_BUILTIN_LIBS])
AC_CONFIG_FILES([clish/plugin_builtin.c])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
