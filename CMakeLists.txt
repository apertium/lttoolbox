PROJECT (apertium-3.0)

INCLUDE(CheckSymbolExists)

INCLUDE_DIRECTORIES (.)

IF (WIN32)
	SET (WIN32_DIR "${PROJECT_SOURCE_DIR}/lttoolbox/win32")
	INCLUDE_DIRECTORIES (${WIN32_DIR})
	SET (EXTRA_SOURCES "${WIN32_DIR}/getopt.c" "${WIN32_DIR}/libgen.c")

	SET (LIBXML2_BASE        "C:/Program Files/LibXML")
	SET (LIBXML2_INCLUDE_DIR "${LIBXML2_BASE}/include")
	SET (LIBXML2_LIBRARIES   "${LIBXML2_BASE}/lib/libxml2.lib")

	ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
ENDIF (WIN32)

SET (PACKAGE_BUGREPORT sortiz@users.sourceforge.net)
SET (PACKAGE_NAME      lttoolbox/lttoolbox.h)
SET (PACKAGE_STRING    lttoolbox/lttoolbox.h 3.0.1)
SET (PACKAGE_TARNAME   lttoolbox-lttoolbox-h)
SET (PACKAGE_VERSION   3.0.1)

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

CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/lttoolbox/lttools_config.h.cmake_in ${PROJECT_SOURCE_DIR}/lttoolbox/lttools_config.h)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
ADD_SUBDIRECTORY (lttoolbox)

