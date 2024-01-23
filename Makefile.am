ACLOCAL_AMFLAGS=-I m4

SUBDIRS = $(PACKAGE_NAME) locales
DIST_SUBDIRS = $(PACKAGE_NAME)

if HAVE_PYTHON_BINDINGS
SUBDIRS += python
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lttoolbox.pc

EXTRA_DIST=autogen.sh tests

# TODO: the below will use python3 if you run it on Arch Linux with no python2 installed
test: tests/run_tests.py
	$(PYTHON) $<

export LOCALES_DIR=$(datadir)/$(PACKAGE_NAME)
export ALT_I18N_DATA=$(LOCALES_DIR)/lttoolbox.dat
