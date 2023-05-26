# CMakeList.txt : CMake project for test_server, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.25.0)

project(linux_simple VERSION 1.0.23)

set(
	SOVERSION 1.0.23
)

set (
	SOURCES
	${CMAKE_SOURCE_DIR}/src/core/linux_simple/crash_investigator_linux_simple_alloc_free.c
    ${CMAKE_SOURCE_DIR}/src/core/linux_simple/crash_investigator_linux_simple_alloc_free_inc.c
    ${CMAKE_SOURCE_DIR}/src/core/linux_simple/crash_investigator_linux_simple_analyze.cpp
	${CMAKE_SOURCE_DIR}/contrib/cpputils/contrib/cinternal/src/core/cinternal_core_hash_lhash.c
	${CMAKE_SOURCE_DIR}/contrib/stack_investigator/src/core/stack_investigator_backtrace_common.c
	${CMAKE_SOURCE_DIR}/contrib/stack_investigator/src/core/stack_investigator_backtrace_unix.c
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# For shared library type targets VERSION specifies build version and SOVERTION specifies SONAME.
# When building or installing appropriate symlinks are created if the platform supports
# symlinks and the linker supports so-names.
# If only one of both is specified the missing is assumed to have the same version number.
# For executables VERSION can be used to specify the build version.
# When building or installing appropriate symlinks are created if the platform supports symlinks.
set_target_properties(
	${PROJECT_NAME} PROPERTIES
	VERSION ${CMAKE_PROJECT_VERSION}
)

# include directories
set (
	INCLUDEDIRS
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/contrib/cpputils/contrib/cinternal/include
	${CMAKE_SOURCE_DIR}/contrib/stack_investigator/include
)
target_include_directories("${PROJECT_NAME}" BEFORE PRIVATE ${INCLUDEDIRS})
target_link_libraries(${PROJECT_NAME} dl)
