cmake_minimum_required(VERSION 3.10)

project(filedlg)


set(SRC 
	src/nfd_common.c
	src/nfd_win.cpp
	#src/nfd_zenity.c
)
	  
add_library(filedlg ${SRC})

target_include_directories(filedlg PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/src/include)

