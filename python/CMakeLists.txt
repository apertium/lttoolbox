find_package(SWIG 3.0 REQUIRED)
find_package(Python 3.8 REQUIRED)
set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})

get_directory_property(_defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
string(REPLACE ";" " -D" defs "-D${_defs}")

set(PYTHON_FILE "lttoolbox.py")
set(CPP_WRAP_FILE "lttoolbox_wrap.cpp")
set(top_srcdir ${CMAKE_SOURCE_DIR})
set(BUILD_LIBDIR $<TARGET_FILE_DIR:lttoolbox>)
set(CXXFLAGS "${CMAKE_CXX_FLAGS} ${defs}")
set(PACKAGE ${PROJECT_NAME})
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(LIBXML_CFLAGS ${LIBXML2_INCLUDE_DIR})
set(ICU_CFLAGS ${ICU_INCLUDE_DIR})

configure_file(lttoolbox.i.in lttoolbox.i @ONLY)
configure_file(setup.py.in setup.py @ONLY)
file(GENERATE OUTPUT setup.py INPUT ${CMAKE_CURRENT_BINARY_DIR}/setup.py) # In CMake 3.19, add: TARGET lttoolbox

add_custom_command(OUTPUT ${CPP_WRAP_FILE} ${PYTHON_FILE}
	COMMAND ${PYTHON_EXECUTABLE} setup.py build
	COMMENT "Building ${PYTHON_FILE}"
	DEPENDS lttoolbox
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
