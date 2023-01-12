cmake_minimum_required(VERSION 3.10)

project(blue_noise)


set(SRC 
	generate_blue_noise.cpp
	generate_blue_noise.h
)
	  
add_library(blue_noise ${SRC})

target_include_directories(blue_noise PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR})

