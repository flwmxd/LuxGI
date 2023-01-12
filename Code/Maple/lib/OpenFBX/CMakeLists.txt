cmake_minimum_required(VERSION 3.10)

project(OpenFBX)

set(_SOURCES
    src/miniz.c
	src/ofbx.cpp
)

add_library(OpenFBX STATIC ${_SOURCES})

target_include_directories(OpenFBX PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/include")


