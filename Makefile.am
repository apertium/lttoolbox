ACLOCAL_AMFLAGS=-I m4

SUBDIRS = $(GENERIC_LIBRARY_NAME)
DIST_SUBDIRS = $(GENERIC_LIBRARY_NAME)

if HAVE_PYTHON_BINDINGS
SUBDIRS += python
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lttoolbox.pc

EXTRA_DIST=autogen.sh utf8 tests

# TODO: the below will use python3 if you run it on Arch Linux with no python2 installed
test: tests/run_tests.py
	$(PYTHON) $<
