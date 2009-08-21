cmake_minimum_required(VERSION 2.6)

PROJECT (apertium-3.1)

INCLUDE(CheckSymbolExists)

INCLUDE_DIRECTORIES (.)

IF (WIN32)
	SET (WIN32_DIR "${PROJECT_SOURCE_DIR}/lttoolbox/win32")
	INCLUDE_DIRECTORIES (${WIN32_DIR})
	SET (EXTRA_SOURCES "${WIN32_DIR}/getopt.c" "${WIN32_DIR}/libgen.c")

	SET (LIBXML2_BASE        "C:/Program Files/LibXML")
	SET (LIBXML2_INCLUDE_DIR "${LIBXML2_BASE}/include")
	SET (LIBXML2_LIBRARIES   "${LIBXML2_BASE}/lib/libxml2.lib")

	SET (ICONV_BASE        "C:/Program Files/iconv")
	SET (ICONV_INCLUDE_DIR "${ICONV_BASE}/include")
	SET (ICONV_LIBRARIES   "${ICONV_BASE}/lib/iconv.lib")

	ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
ENDIF (WIN32)

SET (PACKAGE_BUGREPORT sortiz@users.sourceforge.net)
SET (PACKAGE_NAME      lttoolbox/lttoolbox.h)
SET (PACKAGE_STRING    lttoolbox/lttoolbox.h 3.1.2)
SET (PACKAGE_TARNAME   lttoolbox-lttoolbox-h)
SET (PACKAGE_VERSION   3.1.2)

MACRO (BOOL_CHECK_SYMBOL_EXISTS SYMBOL HEADER VAR)
	CHECK_SYMBOL_EXISTS (${SYMBOL} ${HEADER} ${VAR})
	IF (${VAR})
		SET (${VAR} "1")
	ELSE (${VAR})
		SET (${VAR} "0")
	ENDIF (${VAR})
ENDMACRO (BOOL_CHECK_SYMBOL_EXISTS)

BOOL_CHECK_SYMBOL_EXISTS (fputs_unlocked  stdio.h HAVE_DECL_FPUTS_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fgetc_unlocked  stdio.h HAVE_DECL_FGETC_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fputc_unlocked  stdio.h HAVE_DECL_FPUTC_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fwrite_unlocked stdio.h HAVE_DECL_FWRITE_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fread_unlocked  stdio.h HAVE_DECL_FREAD_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fgetwc_unlocked stdio.h HAVE_DECL_FGETWC_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fputwc_unlocked stdio.h HAVE_DECL_FPUTWC_UNLOCKED)
BOOL_CHECK_SYMBOL_EXISTS (fputws_unlocked stdio.h HAVE_DECL_FPUTWS_UNLOCKED)

BOOL_CHECK_SYMBOL_EXISTS (_fputs_nolock  stdio.h HAVE_DECL_FPUTS_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fgetc_nolock  stdio.h HAVE_DECL_FGETC_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fputc_nolock  stdio.h HAVE_DECL_FPUTC_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fwrite_nolock stdio.h HAVE_DECL_FWRITE_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fread_nolock  stdio.h HAVE_DECL_FREAD_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fgetwc_nolock stdio.h HAVE_DECL_FGETWC_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fputwc_nolock stdio.h HAVE_DECL_FPUTWC_NOLOCK)
BOOL_CHECK_SYMBOL_EXISTS (_fputws_nolock stdio.h HAVE_DECL_FPUTWS_NOLOCK)

CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/lttoolbox/lttoolbox_config.h.cmake_in ${PROJECT_SOURCE_DIR}/lttoolbox/lttoolbox_config.h)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
ADD_SUBDIRECTORY (lttoolbox)

