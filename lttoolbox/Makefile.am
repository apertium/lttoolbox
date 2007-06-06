
h_sources = alphabet.h buffer.h compiler.h compression.h endian_util.h \
            entry_token.h expander.h fst_processor.h lt_locale.h ltstr.h \
            match_exe.h match_node.h match_state.h my_stdio.h node.h \
            pattern_list.h pool.h regexp_compiler.h sorted_vector.h state.h \
            transducer.h trans_exe.h xml_parse_util.h lttools_config.h
cc_sources = alphabet.cc compiler.cc compression.cc entry_token.cc \
             expander.cc fst_processor.cc lt_locale.cc match_exe.cc \
             match_node.cc match_state.cc node.cc pattern_list.cc \
             regexp_compiler.cc sorted_vector.cc state.cc transducer.cc \
             trans_exe.cc xml_parse_util.cc

library_includedir = $(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

bin_PROGRAMS = lt-comp lt-proc lt-expand
instdir = lttoolbox

lib_LTLIBRARIES= liblttoolbox-3.0.la
liblttoolbox_3_0_la_SOURCES= $(h_sources) $(cc_sources)
liblttoolbox_3_0_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

lttoolboxdir = $(prefix)/lttoolbox-$(GENERIC_API_VERSION)
lttoolboxinclude = $(prefix)/include
lttoolboxlib = $(prefix)/lib 

lttoolbox_DATA = dix.dtd

lt_comp_SOURCES = lt_comp.cc  
lt_comp_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_comp_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)

lt_proc_SOURCES = lt_proc.cc
lt_proc_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_proc_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)

lt_expand_SOURCES = lt_expand.cc
lt_expand_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_expand_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)


man_MANS = lt-comp.1 lt-expand.1 lt-proc.1

INCLUDES = -I$(top_srcdir) $(LTTOOLBOX_CFLAGS)
CLEANFILES = *~

EXTRA_DIST = dix.dtd $(man_MANS)
