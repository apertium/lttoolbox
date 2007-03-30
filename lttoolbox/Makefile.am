
h_sources = Alphabet.H RegExpCompiler.H EntryToken.H Ltstr.H Compiler.H \
            Compression.H Transducer.H  LtLocale.H \
            MyStdio.H Buffer.H FSTProcessor.H Ltstr.H \
            TransExe.H State.H Node.H Pool.H LttoolsConfig.H \
            Expander.H XMLParseUtil.H EndianUtil.H \
            PatternList.H  MatchNode.H MatchState.H MatchExe.H \
            SortedVector.H 
cc_sources = Alphabet.C RegExpCompiler.C EntryToken.C Compiler.C \
             Compression.C Transducer.C LtLocale.C FSTProcessor.C TransExe.C \
             State.C Node.C Expander.C XMLParseUtil.C \
             PatternList.C MatchNode.C MatchState.C MatchExe.C \
             SortedVector.C

library_includedir = $(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

bin_PROGRAMS = lt-comp lt-proc lt-expand
instdir = lttoolbox

lib_LTLIBRARIES= liblttoolbox-2.0.la
liblttoolbox_2_0_la_SOURCES= $(h_sources) $(cc_sources)
liblttoolbox_2_0_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

lttoolboxdir = $(prefix)/lttoolbox-$(GENERIC_API_VERSION)
lttoolboxinclude = $(prefix)/include
lttoolboxlib = $(prefix)/lib 

lttoolbox_DATA = dix.dtd

lt_comp_SOURCES = ltcomp.C 
lt_comp_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_comp_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)

lt_proc_SOURCES = ltproc.C 
lt_proc_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_proc_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)

lt_expand_SOURCES = ltexpand.C 
lt_expand_LDADD = liblttoolbox-$(GENERIC_API_VERSION).la
lt_expand_LDFLAGS = -llttoolbox-$(GENERIC_API_VERSION) $(LTTOOLBOX_LIBS)


man_MANS = lt-comp.1 lt-expand.1 lt-proc.1

INCLUDES = -I$(top_srcdir) $(LTTOOLBOX_CFLAGS)
CLEANFILES = *~

EXTRA_DIST = dix.dtd $(man_MANS)
