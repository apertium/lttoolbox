set(LIBLTTOOLBOX_HEADERS
	alphabet.h
	att_compiler.h
	buffer.h
	compiler.h
	compression.h
	deserialiser.h
	entry_token.h
	exception.h
	expander.h
	fst_processor.h
	lt_locale.h
	ltstr.h
	match_exe.h
	match_node.h
	match_state.h
	my_stdio.h
	node.h
	pattern_list.h
	regexp_compiler.h
	serialiser.h
	sorted_vector.h
	state.h
	string_to_wostream.h
	tmx_compiler.h
	trans_exe.h
	transducer.h
	xml_parse_util.h
	)
set(LIBLTTOOLBOX_SOURCES
	alphabet.cc
	att_compiler.cc
	compiler.cc
	compression.cc
	entry_token.cc
	expander.cc
	fst_processor.cc
	lt_locale.cc
	match_exe.cc
	match_node.cc
	match_state.cc
	node.cc
	pattern_list.cc
	regexp_compiler.cc
	sorted_vector.cc
	state.cc
	tmx_compiler.cc
	trans_exe.cc
	transducer.cc
	xml_parse_util.cc
	${LIBLTTOOLBOX_HEADERS}
	)
if(WIN32)
	set(LIBLTTOOLBOX_SOURCES
		win32/libgen.c
		win32/libgen.h
		win32/regex.c
		win32/regex.h
		win32/unistd.h
		${CMAKE_SOURCE_DIR}/utf8/utf8_fwrap.h
		${LIBLTTOOLBOX_SOURCES}
		)
	if(NOT VCPKG_TOOLCHAIN)
		set(LIBLTTOOLBOX_SOURCES
			win32/getopt.c
			win32/getopt.h
			${LIBLTTOOLBOX_SOURCES}
			)
	endif()
else()
	set(GETOPT)
endif()

set(LibLttoolbox "lttoolbox${PROJECT_VERSION_MAJOR}-${API_VERSION}")
add_library(${LibLttoolbox} ${LIBLTTOOLBOX_SOURCES})
target_compile_definitions(${LibLttoolbox} PRIVATE LTTOOLBOX_EXPORTS)
set_target_properties(${LibLttoolbox} PROPERTIES SOVERSION ${SOVERSION})
target_link_libraries(${LibLttoolbox} ${LIBXML2_LIBRARIES})

add_executable(lt-print lt_print.cc)
target_link_libraries(lt-print ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-trim lt_trim.cc)
target_link_libraries(lt-trim ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-comp lt_comp.cc)
target_link_libraries(lt-comp ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-proc lt_proc.cc)
target_link_libraries(lt-proc ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-expand lt_expand.cc)
target_link_libraries(lt-expand ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-tmxcomp lt_tmxcomp.cc)
target_link_libraries(lt-tmxcomp ${LibLttoolbox} ${GETOPT_LIB})

add_executable(lt-tmxproc lt_tmxproc.cc)
target_link_libraries(lt-tmxproc ${LibLttoolbox} ${GETOPT_LIB})

if(BUILD_TESTING)
	add_test(NAME tests COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tests/run_tests.py" $<TARGET_FILE_DIR:lt-comp>)
	set_tests_properties(tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED")
endif()

install(TARGETS ${LibLttoolbox}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${LIBLTTOOLBOX_HEADERS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lttoolbox-${API_VERSION}/lttoolbox)
install(TARGETS lt-print lt-trim lt-comp lt-proc lt-expand lt-tmxcomp lt-tmxproc
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES dix.dtd dix.rng dix.rnc acx.rng xsd/dix.xsd xsd/acx.xsd
	DESTINATION ${CMAKE_INSTALL_DATADIR}/lttoolbox)

install(FILES lt-comp.1 lt-expand.1 lt-proc.1 lt-tmxcomp.1 lt-tmxproc.1 lt-print.1 lt-trim.1
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
