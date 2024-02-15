AC_PREREQ(2.52)

m4_define([PKG_VERSION_MAJOR], [3])
m4_define([PKG_VERSION_MINOR], [7])
m4_define([PKG_VERSION_PATCH], [7])

# Bump if the ABI (not API) changed in a backwards-incompatible manner
m4_define([PKG_VERSION_ABI], [3])

AC_INIT([lttoolbox], [PKG_VERSION_MAJOR.PKG_VERSION_MINOR.PKG_VERSION_PATCH], [apertium-stuff@lists.sourceforge.net], [lttoolbox], [https://wiki.apertium.org/wiki/Lttoolbox])

VERSION=$PACKAGE_VERSION
VERSION_ABI=PKG_VERSION_ABI

AC_SUBST(PACKAGE_NAME)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(VERSION_ABI)

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
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stddef.h])
AC_CHECK_HEADER(["$HOMEBREW_PREFIX/include/utf8cpp/utf8.h"], [CPPFLAGS="-I$HOMEBREW_PREFIX/include/utf8cpp/ $CPPFLAGS"], [
  AC_CHECK_HEADER([utf8cpp/utf8.h], [CPPFLAGS="-I/usr/include/utf8cpp/ $CPPFLAGS"], [
    AC_CHECK_HEADER([utf8.h], [], [AC_MSG_ERROR([You don't have utfcpp installed.])])
  ])
])

# Checks for POSIX thread support
AX_PTHREAD([], [AC_MSG_ERROR([Can't find libpthread])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CHECK_DECLS([fread_unlocked, fwrite_unlocked, fgetc_unlocked, fputc_unlocked, fputs_unlocked, fmemopen])

AC_CHECK_FUNCS([setlocale strdup getopt_long])

AM_CONDITIONAL([WINDOWS], [test x$version_type = xwindows])

# Require highest supported C++ standard
AC_LANG(C++)
for version in 23 2b 20 2a 17; do
  version_flag="-std=c++${version}"
  AX_CHECK_COMPILE_FLAG([${version_flag}], [break], [version_flag=none])
done
AS_IF([test "$version_flag" = none], [
  AC_MSG_ERROR([Could not enable at least C++17 - upgrade your compiler])
])
CXXFLAGS="$CXXFLAGS ${version_flag}"

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <cstddef>
#include <cstdint>
#include <type_traits>
]],[[
static_assert(!std::is_same<size_t,uint32_t>::value, "size_t == uint32_t");
static_assert(!std::is_same<size_t,uint64_t>::value, "size_t == uint64_t");
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
