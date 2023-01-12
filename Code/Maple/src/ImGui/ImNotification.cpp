//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ImNotification.h"
#include <imgui.h>
#include <imgui_notify.h>

namespace maple
{
	auto ImNotification::onImGui() -> void
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);                                                           // Round borders
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f));        // Background color
		ImGui::RenderNotifications();                                                                                     // <-- Here we render all notifications
		ImGui::PopStyleVar(1);                                                                                            // Don't forget to Pop()
		ImGui::PopStyleColor(1);
	}

	auto ImNotification::makeNotification(const std::string &title, const std::string &str, const Type type, int32_t time) -> void
	{
		ImGuiToast toast((uint32_t) type, time);        // <-- content can also be passed here as above
		toast.set_title(title.c_str());
		toast.set_content(str.c_str());
		ImGui::InsertNotification(toast);
	}
};        // namespace maple
