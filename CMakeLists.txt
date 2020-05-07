cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(lttoolbox
	VERSION 3.5.2
	LANGUAGES CXX C
	)
set(VERSION ${PROJECT_VERSION})
set(API_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(SOVERSION 1.0.0)
set(PACKAGE_BUGREPORT "apertium-stuff@lists.sourceforge.net")

add_definitions("-DPACKAGE_VERSION=\"${PROJECT_VERSION}\"")

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(MASTER_PROJECT ON)
endif ()

# Release or Debug
if(MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH ON)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Set to OFF to use static library" ON)
option(BUILD_TESTING "Set to OFF to disable tests" ON)
option(ENABLE_PYTHON_BINDINGS "Set to ON to build the Python wrapper" OFF)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++latest /Zc:__cplusplus /permissive- /W4 /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
	set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else()
	set(_FLAGS_COMMON "-Wall -Wextra -Wno-missing-field-initializers -Wno-deprecated -Wno-unused-parameter -fPIC")

	include(CheckCCompilerFlag)
	include(CheckCXXCompilerFlag)

	foreach(flag "-Wno-unused-result" "-flto")
		string(REGEX REPLACE "[^A-Za-z0-9]" "-" _flag ${flag})
		CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(_FLAGS_COMMON "${_FLAGS_COMMON} ${flag}")
		endif()
	endforeach()
	if(COMPILER_SUPPORTS_flto)
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_FLAGS_COMMON} -fvisibility-inlines-hidden")

	# Enable latest possible C standard
	foreach(flag "-std=c2x" "-std=c11" "-std=c1x" "-std=c99")
		string(REGEX REPLACE "[^a-z0-9]" "-" _flag ${flag})
		CHECK_C_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
			break()
		endif()
	endforeach()

	# Require latest possible C++ standard
	foreach(flag "-std=c++20" "-std=c++2a" "-std=c++17" "-std=c++1z" "-std=c++14" "-std=c++1y")
		string(REGEX REPLACE "[^a-z0-9]" "-" _flag ${flag})
		CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
			set(_ENABLED_CXX ${flag})
			break()
		endif()
	endforeach()
	if(NOT _ENABLED_CXX)
		message(FATAL_ERROR "Could not enable at least C++14 (C++1y) - upgrade your compiler")
	endif()

	# Generate pkg-config file
	set(prefix      ${CMAKE_INSTALL_PREFIX})
	set(exec_prefix ${CMAKE_INSTALL_PREFIX})
	set(libdir      ${CMAKE_INSTALL_LIBDIR})
	set(includedir  ${CMAKE_INSTALL_INCLUDEDIR})
	configure_file(lttoolbox.pc.in lttoolbox.pc @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lttoolbox.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
endif()

try_compile(SIZET_NOT_CSTDINT ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lttoolbox/check-cstdint.cc)
if(SIZET_NOT_CSTDINT)
	add_definitions(-DSIZET_NOT_CSTDINT)
endif()

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

if(WIN32)
	add_definitions(-D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
	add_definitions(-DSTDC_HEADERS -DREGEX_MALLOC)
	include_directories("lttoolbox/win32")
	include_directories("utf8")
else()
	add_definitions(-D_POSIX_C_SOURCE=200112 -D_GNU_SOURCE)
endif()

# Unlocked I/O functions
include(CheckSymbolExists)
set(CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=200112 -D_GNU_SOURCE)
foreach(func fread_unlocked fwrite_unlocked fgetc_unlocked fputc_unlocked fputs_unlocked fgetwc_unlocked fputwc_unlocked fputws_unlocked)
	string(TOUPPER ${func} _uc)
	CHECK_SYMBOL_EXISTS(${func} "stdio.h" HAVE_DECL_${_uc})
	if(HAVE_DECL_${_uc})
		add_definitions(-DHAVE_DECL_${_uc})
	endif()
endforeach()
unset(CMAKE_REQUIRED_DEFINITIONS)

# getopt
find_path(GETOPT_INCLUDE getopt.h)
include_directories(${GETOPT_INCLUDE})
if(VCPKG_TOOLCHAIN)
	find_library(GETOPT_LIB NAMES getopt)
	add_definitions(-DHAVE_GETOPT_LONG)
else()
	set(GETOPT_LIB)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_TESTING)
	enable_testing()
	find_package(PythonInterp 3.5 REQUIRED)
	set(ENV{CTEST_OUTPUT_ON_FAILURE} 1)
	set(CMAKE_CTEST_ARGUMENTS "-VV")
endif()

add_subdirectory(lttoolbox)

if(ENABLE_PYTHON_BINDINGS)
	add_subdirectory(python)
endif()
