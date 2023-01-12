cmake_minimum_required(VERSION 3.4.1)

project(Maple-Engine)

get_filename_component(LIB_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/Maple/lib
                       ABSOLUTE)

get_filename_component(GAME_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/Game/src ABSOLUTE)


get_filename_component(ASSET_DIR
					  ${CMAKE_SOURCE_DIR}/../Assets
                      ABSOLUTE)					   


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")


option(ENGINE_AS_LIBRARY "build engine as dynamic library" OFF)
option(MAPLE_OPENGL "Opengl as the default renderer" ON)
option(MAPLE_VULKAN "Vulkan as the default renderer" OFF)

if(ENGINE_AS_LIBRARY)
	add_definitions(-DMAPLE_DYNAMIC)
endif()

if(MAPLE_OPENGL)
	add_definitions(-DMAPLE_DYNAMIC)
endif()

if(MAPLE_VULKAN)
	add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
	add_definitions(-DMAPLE_VULKAN)
endif()

add_subdirectory(Maple)

file(GLOB GAME_APP_SRC
	${GAME_SRC_DIR}/*.cpp
	${GAME_SRC_DIR}/*.h	
)

if (${Target} MATCHES "Windows")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_compile_options("/std:c++17")
add_definitions(
	-D_CRT_SECURE_NO_WARNINGS 
	-D_CRT_RAND_S 
	-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 
	-DPLATFORM_WINDOWS 
	-DPLATFORM_DESKTOP
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Maple")

source_group(TREE ${GAME_SRC_DIR} FILES ${GAME_APP_SRC})

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/Game.rc")

add_executable(Game ${GAME_APP_SRC} ${APP_ICON_RESOURCE_WINDOWS})

set_property(TARGET Game PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${ASSET_DIR})

set_target_properties(Game PROPERTIES COMPILE_FLAGS "/MP")


target_include_directories(Game PUBLIC
	${GAME_APP_SRC}
	${LIB_SRC_DIR}/opengl/include
	${LIB_SRC_DIR}/imgui/src
	${LIB_SRC_DIR}/spdlog/include
	${LIB_SRC_DIR}/stb_image
	${LIB_SRC_DIR}/vulkan/include
	${LIB_SRC_DIR}/tinyobjloader
	${LIB_SRC_DIR}/glm
	${LIB_SRC_DIR}/entt
	${LIB_SRC_DIR}/SPIRV-Cross
	${LIB_SRC_DIR}/ktx/include
	${LIB_SRC_DIR}/ktx/other_include
	${LIB_SRC_DIR}/cereal/include
	${LIB_SRC_DIR}/glad/include
	${LIB_SRC_DIR}/ecs
	${LIB_SRC_DIR}/mio
	${LIB_SRC_DIR}/OpenFBX
)

target_link_libraries(
	Game 
	MapleEngine
)

add_custom_target(LibCopy
	COMMAND powershell Copy-Item ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/*.dll  ${ASSET_DIR} -Force
	COMMENT "Copying binaries ............"
)

add_dependencies(LibCopy MapleEngine)

endif()


