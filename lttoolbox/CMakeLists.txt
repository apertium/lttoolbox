set(LIBLTTOOLBOX_HEADERS
	acx.h
	alphabet.h
	att_compiler.h
	buffer.h
	cli.h
	compiler.h
	compression.h
	deserialiser.h
	entry_token.h
	exception.h
	expander.h
	file_utils.h
	fst_processor.h
	input_file.h
	lt_locale.h
	match_exe.h
	match_node.h
	match_state.h
	my_stdio.h
	node.h
	pattern_list.h
	regexp_compiler.h
	serialiser.h
	sorted_vector.h
	sorted_vector.hpp
	state.h
	stream_reader.h
	string_utils.h
	symbol_iter.h
	tmx_compiler.h
	transducer.h
	trans_exe.h
	ustring.h
	xml_parse_util.h
	xml_walk_util.h
	)
set(LIBLTTOOLBOX_SOURCES
	acx.cc
	alphabet.cc
	att_compiler.cc
	cli.cc
	compiler.cc
	compression.cc
	entry_token.cc
	expander.cc
	file_utils.cc
	fst_processor.cc
	input_file.cc
	lt_locale.cc
	match_exe.cc
	match_node.cc
	match_state.cc
	node.cc
	pattern_list.cc
	regexp_compiler.cc
	sorted_vector.cc
	state.cc
	stream_reader.cc
	string_utils.cc
	symbol_iter.cc
	tmx_compiler.cc
	transducer.cc
	trans_exe.cc
	ustring.cc
	xml_parse_util.cc
	xml_walk_util.cc
	${LIBLTTOOLBOX_HEADERS}
	)
if(MSVC)
	set(LIBLTTOOLBOX_SOURCES
		win32/libgen.c
		win32/libgen.h
		win32/regex.c
		win32/regex.h
		win32/unistd.h
		${LIBLTTOOLBOX_SOURCES}
		)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(lttoolbox ${LIBLTTOOLBOX_SOURCES})
target_compile_definitions(lttoolbox PRIVATE LTTOOLBOX_EXPORTS)
set_target_properties(lttoolbox PROPERTIES VERSION ${VERSION_ABI}.0.0 SOVERSION ${VERSION_ABI})
target_link_libraries(lttoolbox ${LIBXML2_LIBRARIES} ${ICU_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GETOPT_LIB})

add_executable(lt-append lt_append.cc)
target_link_libraries(lt-append lttoolbox ${GETOPT_LIB})

add_executable(lt-print lt_print.cc)
target_link_libraries(lt-print lttoolbox ${GETOPT_LIB})

add_executable(lt-trim lt_trim.cc)
target_link_libraries(lt-trim lttoolbox ${GETOPT_LIB})

add_executable(lt-compose lt_compose.cc)
target_link_libraries(lt-compose lttoolbox ${GETOPT_LIB})

add_executable(lt-comp lt_comp.cc)
target_link_libraries(lt-comp lttoolbox ${GETOPT_LIB})

add_executable(lt-proc lt_proc.cc)
target_link_libraries(lt-proc lttoolbox ${GETOPT_LIB})

add_executable(lt-merge lt_merge.cc)
target_link_libraries(lt-merge lttoolbox ${GETOPT_LIB})

add_executable(lt-expand lt_expand.cc)
target_link_libraries(lt-expand lttoolbox ${GETOPT_LIB})

add_executable(lt-paradigm lt_paradigm.cc)
target_link_libraries(lt-paradigm lttoolbox ${GETOPT_LIB})

add_executable(lt-tmxcomp lt_tmxcomp.cc)
target_link_libraries(lt-tmxcomp lttoolbox ${GETOPT_LIB})

add_executable(lt-tmxproc lt_tmxproc.cc)
target_link_libraries(lt-tmxproc lttoolbox ${GETOPT_LIB})

add_executable(lt-invert lt_invert.cc)
target_link_libraries(lt-invert lttoolbox ${GETOPT_LIB})

add_executable(lt-restrict lt_restrict.cc)
target_link_libraries(lt-restrict lttoolbox ${GETOPT_LIB})

add_executable(lt-apply-acx lt_apply_acx.cc)
target_link_libraries(lt-apply-acx lttoolbox ${GETOPT_LIB})

if(BUILD_TESTING)
	add_test(NAME tests COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tests/run_tests.py" $<TARGET_FILE_DIR:lt-comp>)
	set_tests_properties(tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED")
endif()

if(WIN32)
	add_custom_target(lsx-comp ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:lt-comp> lsx-comp.exe DEPENDS lt-comp)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lsx-comp.exe DESTINATION ${CMAKE_INSTALL_BINDIR})
	add_custom_target(lsx-comp-man ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/lt-comp.1 lsx-comp.1 DEPENDS lt-comp)
else()
	add_custom_target(lsx-comp ALL COMMAND ${CMAKE_COMMAND} -E create_symlink lt-comp lsx-comp DEPENDS lt-comp)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lsx-comp DESTINATION ${CMAKE_INSTALL_BINDIR})
	add_custom_target(lsx-comp-man ALL COMMAND ${CMAKE_COMMAND} -E create_symlink lt-comp.1 lsx-comp.1 DEPENDS lt-comp)
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lsx-comp.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install(TARGETS lttoolbox
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${LIBLTTOOLBOX_HEADERS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lttoolbox)
install(TARGETS lt-append lt-print lt-trim lt-compose lt-comp lt-proc lt-expand lt-paradigm lt-tmxcomp lt-tmxproc lt-invert lt-restrict lt-apply-acx
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES dix.dtd dix.rng dix.rnc acx.rng xsd/dix.xsd xsd/acx.xsd
	DESTINATION ${CMAKE_INSTALL_DATADIR}/lttoolbox)

install(FILES lt-append.1 lt-comp.1 lt-expand.1 lt-paradigm.1 lt-proc.1 lt-tmxcomp.1 lt-tmxproc.1 lt-print.1 lt-trim.1 lt-compose.1
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
