cmake_minimum_required(VERSION 3.10)

project(opengl)

target_include_directories(opengl PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>")

