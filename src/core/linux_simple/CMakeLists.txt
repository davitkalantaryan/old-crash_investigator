# CMakeList.txt : CMake project for linux_simple, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(linux_simple VERSION 1.0.22 DESCRIPTION "Library to analyze memory leaks")

message("\${CMAKE_SOURCE_DIR} = " ${CMAKE_SOURCE_DIR})


set (
  INCLUDEDIRS
  ${CMAKE_SOURCE_DIR}/../../../include
  ${CMAKE_SOURCE_DIR}/../../../contrib/cpputils/contrib/cinternal/include
  ${CMAKE_SOURCE_DIR}/../../../contrib/stack_investigator/include
)

# Add source to this project's executable.
add_library (linux_simple SHARED "../linux_simple/crash_investigator_linux_simple_alloc_free.c" "crash_investigator_linux_simple_alloc_free_inc.c" "crash_investigator_linux_simple_analyze.cpp")


target_include_directories("linux_simple" BEFORE PRIVATE ${INCLUDEDIRS})
