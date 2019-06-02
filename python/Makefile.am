cc_sources = alphabet.cc att_compiler.cc compiler.cc compression.cc entry_token.cc \
             expander.cc fst_processor.cc lt_locale.cc match_exe.cc \
             match_node.cc match_state.cc node.cc pattern_list.cc \
             regexp_compiler.cc sorted_vector.cc state.cc transducer.cc \
             trans_exe.cc xml_parse_util.cc tmx_compiler.cc

# SWIG_INTERFACE = analysis.i

EXTRA_DIST = analysis.i

pkgdata_DATA = analysis.py

lib_LTLIBRARIES = libAnalysisPython.la
libAnalysisPython_la_SOURCES = analysis_wrap_python.cpp
libAnalysisPython_la_SOURCES = analysis_wrap_python.cpp
foo_wrap_python.cpp: analysis.i
    $(SWIG) -python $(SWIG_ARGS) -o $@ $<


BUILT_SOURCES = analysis_wrap.cpp analysis.py
CLEANFILES = $(BUILT_SOURCES)

library_includedir = $(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

%_wrap.cpp %.py: %.i  ../src/libdivvun.la
	$(SWIG) -c++ -python -I$(top_srcdir)/src/lib -o $*_wrap.cpp -outdir . $(srcdir)/$*.i
	CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="-Wl,-rpath,${prefix}/lib $(LDFLAGS)" 

