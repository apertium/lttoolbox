
h_sources = acx.h alphabet.h att_compiler.h buffer.h cli.h compiler.h compression.h  \
            deserialiser.h entry_token.h expander.h file_utils.h fst_processor.h input_file.h lt_locale.h \
            match_exe.h match_node.h match_state.h my_stdio.h node.h \
            pattern_list.h regexp_compiler.h serialiser.h sorted_vector.h state.h string_utils.h \
            transducer.h trans_exe.h xml_parse_util.h xml_walk_util.h exception.h tmx_compiler.h \
            ustring.h sorted_vector.hpp
cc_sources = acx.cc alphabet.cc att_compiler.cc cli.cc compiler.cc compression.cc entry_token.cc \
             expander.cc file_utils.cc fst_processor.cc input_file.cc lt_locale.cc match_exe.cc \
             match_node.cc match_state.cc node.cc pattern_list.cc \
             regexp_compiler.cc sorted_vector.cc state.cc string_utils.cc transducer.cc \
             trans_exe.cc xml_parse_util.cc xml_walk_util.cc tmx_compiler.cc ustring.cc

library_includedir = $(includedir)/$(PACKAGE_NAME)-$(VERSION_API)/$(PACKAGE_NAME)
library_include_HEADERS = $(h_sources)

bin_PROGRAMS = lt-comp lt-proc lt-expand lt-paradigm lt-tmxcomp lt-tmxproc lt-print lt-trim lt-append lsx-comp lt-invert lt-restrict
instdir = lttoolbox

lib_LTLIBRARIES= liblttoolbox3.la
liblttoolbox3_la_SOURCES= $(h_sources) $(cc_sources)
liblttoolbox3_la_LDFLAGS= -version-info $(SOVERSION) -release $(VERSION_API)
liblttoolbox3_la_LIBADD= $(ICU_LIBS)

lttoolboxdir = $(prefix)/share/lttoolbox
lttoolboxinclude = $(prefix)/include
lttoolboxlib = $(prefix)/lib

lttoolbox_DATA = dix.dtd dix.rng dix.rnc acx.rng xsd/dix.xsd xsd/acx.xsd

LDADD = liblttoolbox$(VERSION_MAJOR).la
AM_LDFLAGS = -llttoolbox$(VERSION_MAJOR) $(LIBXML_LIBS) $(ICU_LIBS)

lt_append_SOURCES = lt_append.cc
lt_print_SOURCES = lt_print.cc
lt_trim_SOURCES = lt_trim.cc
lt_comp_SOURCES = lt_comp.cc
lt_proc_SOURCES = lt_proc.cc
lt_expand_SOURCES = lt_expand.cc
lt_paradigm_SOURCES = lt_paradigm.cc
lt_tmxcomp_SOURCES = lt_tmxcomp.cc
lt_tmxproc_SOURCES = lt_tmxproc.cc
lsx_comp_SOURCES = lt_comp.cc
lt_invert_SOURCES = lt_invert.cc
lt_restrict_SOURCES = lt_restrict.cc

#lt-validate-dictionary: Makefile.am validate-header.sh
#	@echo "Creating lt-validate-dictionary script"
#	@echo "#!$(BASH)" > $@
#	@cat validate-header.sh >> $@
#	@echo "$(XMLLINT) --dtdvalid $(apertiumdir)/dix.dtd --noout \$$FILE1 && exit 0;" >> $@
#	@echo "exit 1;" >> $@
#	@chmod a+x $@



man_MANS = lt-append.1 lt-comp.1 lt-expand.1 lt-paradigm.1 lt-proc.1 lt-tmxcomp.1 lt-tmxproc.1 lt-print.1 lt-trim.1 lsx-comp.1

INCLUDES = -I$(top_srcdir) $(LIBXML_CFLAGS) $(ICU_CFLAGS)
CLEANFILES = *~

EXTRA_DIST = dix.dtd dix.rng dix.rnc acx.rng xsd/dix.xsd xsd/acx.xsd $(man_MANS)
