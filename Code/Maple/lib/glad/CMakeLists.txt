cmake_minimum_required(VERSION 3.10)

project(glad)

set(_SOURCES
    src/glad.c
	src/glad_wgl.c
)

add_library(glad STATIC ${_SOURCES})

target_include_directories(glad PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/include")


