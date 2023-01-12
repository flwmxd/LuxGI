
cmake_minimum_required(VERSION 3.10)

project(stb_image)

set(SRC stb_image.cpp )

add_library(stb_image ${SRC})

target_include_directories(stb_image PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

