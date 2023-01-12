
#include <imgui.h>
namespace ImGui {
	IMGUI_API bool Spinner(const char* label, float radius, int thickness, const ImU32& color);
	IMGUI_API bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);
};