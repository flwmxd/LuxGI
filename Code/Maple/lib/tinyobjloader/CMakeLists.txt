cmake_minimum_required(VERSION 3.10)
project(tinyobjloader)

set(CMAKE_CXX_STANDARD 17)


add_library(tinyobjloader tiny_obj_loader.cc)


target_include_directories(tinyobjloader PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>")
