cmake_minimum_required(VERSION 3.10)

project(imgui)


set(IMGUI_SRC 
	src/imgui_tables.cpp
	src/imgui_draw.cpp
	src/imgui_widgets.cpp
	src/imgui_widgets_user.cpp
	src/imgui.cpp
	src/imgui_impl_opengl3.cpp
	src/imgui_impl_opengl3.cpp
	src/TextEditor.cpp
	src/imgui_impl_vulkan.cpp
	src/imgui_impl_glfw.cpp
	src/ImGuizmo.cpp
	src/imgui_stacklayout.cpp
	src/implot.cpp
	src/implot_items.cpp
)
	  
#add_library(imgui ${IMGUI_SRC})



#target_include_directories(imgui PUBLIC
#	../opengl/include
#)
#target_include_directories(imgui PUBLIC
#	../vulkan/include
#)

#target_include_directories(imgui PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>")

