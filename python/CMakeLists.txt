find_package(SWIG 3.0 REQUIRED)
find_package(PythonInterp 3.5 REQUIRED)

get_directory_property(_defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
string(REPLACE ";" " -D" defs "-D${_defs}")

set(PYTHON_FILE "lttoolbox.py")
set(CPP_WRAP_FILE "lttoolbox_wrap.cpp")
set(top_srcdir ${CMAKE_SOURCE_DIR})
set(CXXFLAGS "${CMAKE_CXX_FLAGS} ${defs}")
set(PACKAGE ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})

configure_file(lttoolbox.i.in lttoolbox.i @ONLY)
configure_file(setup.py.in setup.py @ONLY)

add_custom_command(OUTPUT ${CPP_WRAP_FILE} ${PYTHON_FILE}
	COMMAND ${PYTHON_EXECUTABLE} setup.py build
	COMMENT "Building ${PYTHON_FILE}"
)

add_custom_target(wrapper ALL
	DEPENDS ${CPP_WRAP_FILE} ${PYTHON_FILE}
	VERBATIM
)

if(NOT PYTHON_INSTALL_PARAMS)
	set(PYTHON_INSTALL_PARAMS "--prefix=${CMAKE_INSTALL_PREFIX} --root=\$ENV{DESTDIR}/")
endif()

set(INSTALL_WRAPPER "${PYTHON_EXECUTABLE} setup.py install ${PYTHON_INSTALL_PARAMS}")
install(CODE "execute_process(COMMAND ${INSTALL_WRAPPER} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
