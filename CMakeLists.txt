# CMakeList.txt : CMake project for test_server, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.25.0)

project(linux_simple VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED)

add_subdirectory(src/core/linux_simple)

# include directories
set (
	INCLUDEDIRS
	${CMAKE_SOURCE_DIR}/include
)
target_include_directories("${PROJECT_NAME}" BEFORE PRIVATE ${INCLUDEDIRS})
