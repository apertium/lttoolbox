#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.52)

m4_define([required_libxml_version], [2.6.17])

AC_INIT([lttoolbox/lttoolbox.h], [3.3.3], [sortiz@users.sourceforge.net])
AC_CONFIG_HEADER([lttoolbox/lttoolbox_config.h])

AC_CANONICAL_SYSTEM

GENERIC_LIBRARY_NAME=lttoolbox

# Release versioning
GENERIC_MAJOR_VERSION=3
GENERIC_MINOR_VERSION=3
GENERIC_MICRO_VERSION=3

# API version (often = GENERIC_MAJOR_VERSION.GENERIC_MINOR_VERSION)
GENERIC_API_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_API_VERSION)
AC_SUBST(GENERIC_MAJOR_VERSION)

# Shared library versioning
GENERIC_LIBRARY_VERSION=0:0:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed

AC_SUBST(GENERIC_LIBRARY_VERSION)
PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_PROG_CXX
AM_PROG_LIBTOOL
AM_SANITY_CHECK
AC_LANG_CPLUSPLUS

CFLAGS="-Wall -Wextra $CFLAGS"
CXXFLAGS="-Wall -Wextra $CXXFLAGS"

AC_ARG_ENABLE(debug,
              [  --enable-debug  Enable "-g -Wall" compiler options],
              [CXXFLAGS="-g -Wall";CFLAGS="-g -Wall"])

AC_ARG_ENABLE(profile,
              [  --enable-profile  Enable "-pg -g -Wall" compiler options],
              [CXXFLAGS="-pg -g -Wall"; CFLAGS="-pg -g -Wall"; LDFLAGS="-pg"])


AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$ac_cv_path_PKG_CONFIG = x
then
  AC_MSG_ERROR([You don't have pkg-config installed])
fi
if test x$ac_cv_path_PKG_CONFIG = xno
then
  AC_MSG_ERROR([You don't have pkg-config installed])
fi

PKG_CHECK_MODULES(LTTOOLBOX, [libxml-2.0 >= required_libxml_version])

# Check for wide strings
AC_DEFUN([AC_CXX_WSTRING],[
  AC_CACHE_CHECK(whether the compiler supports wide strings,
  ac_cv_cxx_wstring,
  [AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string>]],[[
std::wstring test = L"test";
   ]])],
   [ac_cv_cxx_wstring=yes], [ac_cv_cxx_wstring=no])
   AC_LANG_RESTORE
  ])
])

AC_CXX_WSTRING

if test "$ac_cv_cxx_wstring" = no
then
  AC_MSG_ERROR([Missing wide string support])
fi


# Checks for libraries.
AC_CHECK_LIB(xml2, xmlReaderForFile)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stddef.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CHECK_DECLS([fread_unlocked, fwrite_unlocked, fgetc_unlocked, \
fputc_unlocked, fputs_unlocked, fgetwc_unlocked, fputwc_unlocked, fputws_unlocked])

AC_CHECK_FUNCS([setlocale strdup getopt_long])

AM_CONDITIONAL([WINDOWS], [test x$version_type = xwindows])

# Checks for highest supported C++ standard
AC_LANG(C++)
AX_CHECK_COMPILE_FLAG([-std=c++17], [CXXFLAGS="$CXXFLAGS -std=c++17"], [
 AX_CHECK_COMPILE_FLAG([-std=c++1z], [CXXFLAGS="$CXXFLAGS -std=c++1z"], [
  AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXFLAGS="$CXXFLAGS -std=c++14"], [
   AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"], [
    AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS="$CXXFLAGS -std=c++11"], [])
   ])
  ])
 ])
])

AM_PATH_PYTHON([2], [], [AC_MSG_WARN([Can't run 'make test' without Python installed.])])

AC_OUTPUT([Makefile lttoolbox.pc lttoolbox/Makefile])
