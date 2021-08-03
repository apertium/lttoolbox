AC_PREREQ(2.52)

m4_define([PKG_VERSION_MAJOR], [3])
m4_define([PKG_VERSION_MINOR], [6])
m4_define([PKG_VERSION_PATCH], [0])

AC_INIT([lttoolbox], [PKG_VERSION_MAJOR.PKG_VERSION_MINOR.PKG_VERSION_PATCH], [apertium-stuff@lists.sourceforge.net], [lttoolbox], [https://wiki.apertium.org/wiki/Lttoolbox])

VERSION=$PACKAGE_VERSION
VERSION_MAJOR=PKG_VERSION_MAJOR
VERSION_API=PKG_VERSION_MAJOR.PKG_VERSION_MINOR
SOVERSION=1:0:0

AC_SUBST(PACKAGE_NAME)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_API)
AC_SUBST(SOVERSION)

AM_INIT_AUTOMAKE
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


PKG_CHECK_MODULES(LIBXML, [libxml-2.0 >= 2.6.17])
PKG_CHECK_MODULES(ICU, [icu-i18n, icu-io, icu-uc])

# Checks for libraries.
AC_CHECK_LIB(xml2, xmlReaderForFile)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stddef.h string_view])

have_sv=""
AC_CHECK_HEADERS([string_view], [have_sv="-DHAVE_STRING_VIEW"], [have_sv=""])
AC_SUBST([have_sv])

AC_CHECK_HEADER([utf8.h], [], [AC_MSG_ERROR([You don't have utfcpp installed.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CHECK_DECLS([fread_unlocked, fwrite_unlocked, fgetc_unlocked, \
fputc_unlocked, fputs_unlocked])

AC_CHECK_FUNCS([setlocale strdup getopt_long])

AM_CONDITIONAL([WINDOWS], [test x$version_type = xwindows])

# Require highest supported C++ standard
AC_LANG(C++)
AX_CHECK_COMPILE_FLAG([-std=c++20], [CXXFLAGS="$CXXFLAGS -std=c++20"], [
 AX_CHECK_COMPILE_FLAG([-std=c++2a], [CXXFLAGS="$CXXFLAGS -std=c++2a"], [
  AX_CHECK_COMPILE_FLAG([-std=c++17], [CXXFLAGS="$CXXFLAGS -std=c++17"], [
   AX_CHECK_COMPILE_FLAG([-std=c++1z], [CXXFLAGS="$CXXFLAGS -std=c++1z"], [
    AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXFLAGS="$CXXFLAGS -std=c++14"], [
     AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"], [
      AC_MSG_ERROR([Could not enable at least C++1y (C++14) - upgrade your compiler])
     ])
    ])
   ])
  ])
 ])
])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <cstddef>
#include <cstdint>
#include <type_traits>
]],[[
using namespace std;
static_assert(!is_same<size_t,uint32_t>::value, "size_t == uint32_t");
static_assert(!is_same<size_t,uint64_t>::value, "size_t == uint64_t");
   ]])], [AC_DEFINE([SIZET_NOT_CSTDINT], [1], [size_t != (uint32_t, uint64_t)])])

AM_PATH_PYTHON([3.4], [], [AC_MSG_WARN([Can't generate SWIG wrapper or run tests without Python])])

AC_CONFIG_FILES([python/lttoolbox.i python/setup.py])

AC_ARG_ENABLE([python-bindings],
              AS_HELP_STRING([--enable-python-bindings],
                             [build python bindings (default=disabled)]),
              [enable_python_bindings=$enableval],
              [enable_python_bindings=no])
AM_CONDITIONAL([HAVE_PYTHON_BINDINGS], [test x$enable_python_bindings = xyes])

AC_ARG_VAR([PYTHON_INSTALL_PARAMS], [Parameters to pass to the Python 3 module install step])
if test "x$PYTHON_INSTALL_PARAMS" = "x"
then
  PYTHON_INSTALL_PARAMS="--prefix=\$(prefix) --root=\$(DESTDIR)/"
fi

AC_OUTPUT([Makefile lttoolbox.pc lttoolbox/Makefile python/Makefile])
