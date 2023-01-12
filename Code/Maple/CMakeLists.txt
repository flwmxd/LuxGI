cmake_minimum_required(VERSION 3.4.1)

project(MapleEngine)


get_filename_component(ENGINE_LIB_SRC_DIR
                       ${CMAKE_CURRENT_LIST_DIR}/lib
                       ABSOLUTE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

if(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    message("Using 64-bit glslangValidator")
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("Using 32-bit glslangValidator")
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

add_subdirectory(lib/spdlog)
add_subdirectory(lib/stb_image)
add_subdirectory(lib/tinyobjloader)
add_subdirectory(lib/SPIRV-Cross)
add_subdirectory(lib/tracy)
add_subdirectory(lib/glad)
add_subdirectory(lib/nativefiledialog)
add_subdirectory(lib/OpenFBX)
add_subdirectory(lib/mio)
add_subdirectory(lib/tracer)
add_subdirectory(lib/noise)

file(GLOB IMGUI_SRC 
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_tables.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_draw.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_widgets.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_widgets_user.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_opengl3.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_opengl3.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_vulkan.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_glfw.cpp
)

file(GLOB_RECURSE SHADERS_GLSL 
	src/Shaders/*.vert 
	src/Shaders/*.frag 
	src/Shaders/*.comp 
	src/Shaders/*.geom 
	src/Shaders/*.tesc 
	src/Shaders/*.tese 
	src/Shaders/*.mesh 
	src/Shaders/*.task 
	src/Shaders/*.rgen
	src/Shaders/*.rchit 
	src/Shaders/*.rahit 
	src/Shaders/*.rmiss
	src/Shaders/*.shader
)

file(GLOB_RECURSE VK_APP_SRC
	src/*.cpp
	src/*.h	
	src/*.inl	
)

#### config OZZ manually 


if(ENGINE_AS_LIBRARY)
	add_definitions(-DMAPLE_DYNAMIC -DMAPLE_ENGINE)
endif()

if(MAPLE_OPENGL)
	add_definitions(-DMAPLE_OPENGL)
endif()

if(MAPLE_VULKAN)
	add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
	add_definitions(-DMAPLE_VULKAN -DUSE_VMA_ALLOCATOR)
endif()


if (${Target} MATCHES "Windows")

	add_compile_options("/std:c++17")
	add_compile_options("/MP")
	add_definitions(
		-D_CRT_SECURE_NO_WARNINGS 
		-D_CRT_RAND_S 
		-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 
		-DPLATFORM_WINDOWS
		-DPLATFORM_DESKTOP
		-DMAPLE_DEBUG
		-DNOMINMAX #conflict between windows and limits
		-DENTT_DISABLE_ASSERT
	)

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${VK_APP_SRC})

	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SHADERS_GLSL})

if(ENGINE_AS_LIBRARY)
	add_library(MapleEngine SHARED ${VK_APP_SRC} ${SHADERS_GLSL} ${IMGUI_SRC} ${OZZ_SRC})
else()
	add_library(MapleEngine STATIC ${VK_APP_SRC} ${SHADERS_GLSL} ${IMGUI_SRC} ${OZZ_SRC})
endif()

	set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

	

	find_package(Vulkan REQUIRED)
	IF (NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
	ELSE()
	    message(STATUS ${Vulkan_LIBRARY})
	ENDIF()

 	target_link_libraries(MapleEngine ${Vulkan_LIBRARY})

	target_include_directories(MapleEngine PUBLIC
		src
		${ENGINE_LIB_SRC_DIR}/opengl/include
		${ENGINE_LIB_SRC_DIR}/imgui/src
		${ENGINE_LIB_SRC_DIR}/spdlog/include
		${ENGINE_LIB_SRC_DIR}/stb_image
		${ENGINE_LIB_SRC_DIR}/vulkan/include
		${ENGINE_LIB_SRC_DIR}/tinyobjloader
		${ENGINE_LIB_SRC_DIR}/glm
		${ENGINE_LIB_SRC_DIR}/entt
		${ENGINE_LIB_SRC_DIR}/SPIRV-Cross
		${ENGINE_LIB_SRC_DIR}/cereal/include
		${ENGINE_LIB_SRC_DIR}/tracy
		${ENGINE_LIB_SRC_DIR}/glad/include
		${ENGINE_LIB_SRC_DIR}/nativefiledialog/include
		${ENGINE_LIB_SRC_DIR}/ecs
		${ENGINE_LIB_SRC_DIR}/tinygltf
		${ENGINE_LIB_SRC_DIR}/mio
		${ENGINE_LIB_SRC_DIR}/vulkan-mem
		${ENGINE_LIB_SRC_DIR}/scope_guard
		${ENGINE_LIB_SRC_DIR}/noise
		${ENGINE_LIB_SRC_DIR}/libacc
		${Vulkan_INCLUDE_DIRS}
	)

	target_link_libraries(MapleEngine 
		${ENGINE_LIB_SRC_DIR}/opengl/lib/${Arch}/glfw3.lib
		${ENGINE_LIB_SRC_DIR}/opengl/lib/${Arch}/opengl32.lib
		tinyobjloader
		spirvCross
		TracyClient
		glad
		filedlg
		OpenFBX
		mio
		tracer
		blue_noise
	)


	find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(MapleEngine OpenMP::OpenMP_CXX)
endif()

	set_target_properties(
	OpenFBX tinyobjloader 
	spirvCross 
	spdlog_headers_for_ide 
	stb_image glad TracyClient filedlg 
	blue_noise
	PROPERTIES FOLDER Library)
	

set_target_properties(
	check
	basicTest
	cSegFault
	cTest
	tracer
	segFaultTest
	PROPERTIES FOLDER TracerLib)
	

foreach(GLSL ${SHADERS_GLSL})

	set(DIR_NAME "")

	string(REPLACE ${CMAKE_CURRENT_LIST_DIR}/src/Shaders "" DIR_NAME ${GLSL})

    set(SPIRV "${LIBRARY_OUTPUT_PATH}/../../../Assets/shaders/spv${DIR_NAME}.spv")

	get_filename_component(FILE_NAME ${GLSL} NAME)

	string(REPLACE ${FILE_NAME} "" DIR_NAME ${DIR_NAME})

if(MAPLE_VULKAN)
   add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRARY_OUTPUT_PATH}/../../../Assets/shaders/spv${DIR_NAME}"
        COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.2 -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
else()
	   add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRARY_OUTPUT_PATH}/../../../Assets/shaders/spv${DIR_NAME}"
        COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.2 -V ${GLSL} -o ${SPIRV} -DGL_NDC
        DEPENDS ${GLSL})
endif()

 

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(MapleShader DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(MapleEngine MapleShader)
	
endif()


