FIND_PACKAGE (LibXml2 REQUIRED)

INCLUDE_DIRECTORIES (${LIBXML2_INCLUDE_DIR})
ADD_DEFINITIONS (${LIBXML2_DEFINITIONS})
INCLUDE_DIRECTORIES (${ICONV_INCLUDE_DIR})

ADD_LIBRARY (lttoolbox3 STATIC alphabet.cc att_compiler.cc compiler.cc compression.cc entry_token.cc
             expander.cc fst_processor.cc lt_locale.cc match_exe.cc
             match_node.cc match_state.cc node.cc pattern_list.cc
             regexp_compiler.cc sorted_vector.cc state.cc transducer.cc
             trans_exe.cc xml_parse_util.cc tmx_compiler.cc ${EXTRA_SOURCES})

TARGET_LINK_LIBRARIES (lttoolbox3 ${LIBXML2_LIBRARIES} ${ICONV_LIBRARIES})

ADD_EXECUTABLE (lt-proc lt_proc.cc)
TARGET_LINK_LIBRARIES (lt-proc lttoolbox3)

ADD_EXECUTABLE (lt-comp lt_comp.cc)
TARGET_LINK_LIBRARIES (lt-comp lttoolbox3)

ADD_EXECUTABLE (lt-expand lt_expand.cc)
TARGET_LINK_LIBRARIES (lt-expand lttoolbox3)

ADD_EXECUTABLE (lt-tmxcomp lt_tmxcomp.cc)
TARGET_LINK_LIBRARIES (lt-tmxcomp lttoolbox3)

ADD_EXECUTABLE (lt-tmxproc lt_tmxproc.cc)
TARGET_LINK_LIBRARIES (lt-tmxproc lttoolbox3)

INSTALL (TARGETS lt-proc lt-comp lt-expand lt-tmxcomp lt-tmxproc lttoolbox3
	 RUNTIME DESTINATION bin
	 LIBRARY DESTINATION lib
	 ARCHIVE DESTINATION lib)

FILE(GLOB HEADERS *.h)

INSTALL (FILES ${HEADERS}
         DESTINATION include/lttoolbox-3.1/lttoolbox)

INSTALL (FILES dix.dtd
         DESTINATION share/lttoolbox-3.1)
