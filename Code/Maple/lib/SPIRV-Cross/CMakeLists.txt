cmake_minimum_required(VERSION 3.10)
project(spirvCross)

set(CMAKE_CXX_STANDARD 17)


add_library(spirvCross
	spirv_cfg.cpp
	spirv_cpp.cpp
	spirv_cross.cpp
	spirv_cross_c.cpp
	spirv_cross_parsed_ir.cpp
	spirv_cross_util.cpp
	spirv_glsl.cpp
	spirv_hlsl.cpp
	spirv_msl.cpp
	spirv_parser.cpp
	spirv_reflect.cpp
)

target_include_directories(spirvCross PUBLIC 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>"
	include
)
