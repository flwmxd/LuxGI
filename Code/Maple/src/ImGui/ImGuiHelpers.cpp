//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "ImGuiHelpers.h"

#include "RHI/Texture.h"

#include <IconsMaterialDesignIcons.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_impl_vulkan.h>

namespace maple
{
	namespace ImGuiHelper
	{
		inline void addUnderLine(ImColor col)
		{
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();
			min.y      = max.y;
			ImGui::GetWindowDrawList()->AddLine(min, max, col, 1.0f);
		}

		auto tooltip(const char *str) -> void
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(str);
				ImGui::EndTooltip();
			}
			ImGui::PopStyleVar();
		}

		auto property(const std::string &name, bool &value) -> bool
		{
			bool updated = false;
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if (ImGui::Checkbox(id.c_str(), &value))
				updated = true;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			return updated;
		}

		auto property(const std::string &name, float &value, float min, float max, PropertyFlag flags, const char *format, float speed) -> bool
		{
			bool updated = false;
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if ((int32_t) flags & (int32_t) PropertyFlag::DragFloat)
			{
				if (ImGui::DragFloat(id.c_str(), &value, speed, min, max, format))
					updated = true;
			}
			else
			{
				if (ImGui::SliderFloat(id.c_str(), &value, min, max, format))
					updated = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, int32_t &value, int32_t min, int32_t max, PropertyFlag flags) -> bool
		{
			bool updated = false;
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;

			if (flags == PropertyFlag::DragInt)
			{
				if (ImGui::DragInt(id.c_str(), &value,1 , min, max))
					updated = true;
			}
			else
			{
				if (ImGui::SliderInt(id.c_str(), &value, min, max))
					updated = true;
			}


			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, glm::vec2 &value, PropertyFlag flags) -> bool
		{
			return ImGuiHelper::property(name, value, -1.0f, 1.0f, flags);
		}

		auto property(const std::string &name, glm::vec2 &value, float min, float max, PropertyFlag flags) -> bool
		{
			bool updated = false;

			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if (ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max))
				updated = true;

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, glm::vec3 &value, PropertyFlag flags) -> bool
		{
			return ImGuiHelper::property(name, value, -1.0f, 1.0f, flags);
		}

		auto property(const std::string &name, glm::vec3 &value, float min, float max, PropertyFlag flags, float speed) -> bool
		{
			bool updated = false;

			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if ((int32_t) flags & (int32_t) PropertyFlag::ColorProperty)
			{
				if (ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs))
					updated = true;
			}
			else if ((int32_t) flags & (int32_t) PropertyFlag::DragFloat)
			{
				if (ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), speed, min, max))
					updated = true;
			}
			else
			{
				if (ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max))
					updated = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, glm::vec4 &value, bool exposeW, PropertyFlag flags) -> bool
		{
			return property(name, value, -1.0f, 1.0f, exposeW, flags);
		}

		auto property(const std::string &name, glm::vec4 &value, float min /*= -1.0f*/, float max /*= 1.0f*/, bool exposeW /*= false*/, PropertyFlag flags /*= PropertyFlag::None*/) -> bool
		{
			bool updated = false;

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if ((int) flags & (int) PropertyFlag::ColorProperty)
			{
				if (ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs))
					updated = true;
			}
			else if ((exposeW ? ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max) : ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max)))
				updated = true;

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, glm::vec3 &value, float min /*= -1.0f*/, float max /*= 1.0f*/, bool exposeW /*= false*/, PropertyFlag flags /*= PropertyFlag::None*/) -> bool
		{
			bool updated = false;

			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if ((int) flags & (int) PropertyFlag::ColorProperty)
			{
				if (ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs))
					updated = true;
			}
			else
			{
				if (ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max))
					updated = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, std::string &value, bool disable) -> bool
		{
			bool updated = false;
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id       = "##" + name;
			static char obj[256] = {};
			memcpy(obj, value.c_str(), value.length() + 1);

			ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
			if (disable)
			{
				flags = ImGuiInputTextFlags_ReadOnly;
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(200, 200, 200)));
			}

			if (ImGui::InputText(id.c_str(), obj, 256))
			{
				updated = true;
				value   = obj;
			}

			if (disable)
				ImGui::PopStyleColor();

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto property(const std::string &name, uint32_t &value, uint32_t min /*= 0*/, uint32_t max /*= 1*/, PropertyFlag flags /*= PropertyFlag::None*/) -> bool
		{
			bool updated = false;
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if (ImGui::SliderInt(id.c_str(), (int32_t *) &value, min, max))
				updated = true;

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return updated;
		}

		auto hyperLink(const std::string &name, const std::string &value, const std::string &hint, const std::function<void()> &callback) -> void
		{
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			ImGui::Text(value.c_str());

			if (value != "")
			{
				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseClicked(0))
					{
						if (callback != nullptr)
						{
							callback();
						}
					}
					addUnderLine(ImColor{238, 83, 23, 255});
					ImGui::SetTooltip(ICON_MDI_LINK "%s", hint.c_str());
				}
				else
				{
					addUnderLine(ImColor{197, 210, 233, 255});
				}
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();
		}

		auto showProperty(const std::string &name, const std::string &value) -> void
		{
			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(value.c_str());
			ImGui::PopItemWidth();
			ImGui::NextColumn();
		}

		auto showProperty(const std::string &name, const glm::vec3 &value) -> void
		{
			auto v = value;

			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			ImGui::InputFloat3(id.c_str(), &v.r, "%.3f", ImGuiInputTextFlags_ReadOnly);

			ImGui::PopItemWidth();
			ImGui::NextColumn();
		}

		auto propertyWithDefault(const std::string &name, float &value, float min /*= -1.0f*/, float max /*= 1.0f*/, float defaultValue /*= 0.f*/, PropertyFlag flags /*= PropertyFlag::None*/, float speed) -> bool
		{
			auto ret = property(name, value, min, max, flags, "%.3f", speed);

			ImGui::PushItemWidth(-1);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0));
			std::string resetCenterPoint = ICON_MDI_UNDO + std::string("##" + name);
			if (ImGui::Button(resetCenterPoint.c_str()))
			{
				ret   = true;
				value = defaultValue;
			}
			ImGui::PopStyleColor();

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return ret;
		}

		auto propertyWithDefault(const std::string &name, glm::vec3 &value, float min /*= -1.0f*/, float max /*= 1.0f*/, const glm::vec3 &defaultValue /*= {}*/, PropertyFlag flags, float speed) -> bool
		{
			auto ret = property(name, value, min, max, flags, speed);

			ImGui::PushItemWidth(-1);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0));
			std::string resetCenterPoint = ICON_MDI_UNDO + std::string("##" + name);
			if (ImGui::Button(resetCenterPoint.c_str()))
			{
				ret   = true;
				value = defaultValue;
			}
			ImGui::PopStyleColor();

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			return ret;
		}

		auto inputFloat(const std::string &name, float &value, float min /*= -1.0f*/, float max /*= 1.0f*/) -> bool
		{
			bool updated = false;

			ImGui::TextUnformatted(name.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			std::string id = "##" + name;
			if (ImGui::InputFloat(id.c_str(), &value, min, max))
				updated = true;

			ImGui::PopItemWidth();
			ImGui::NextColumn();
			return updated;
		}

		auto image(Texture *texture, const glm::vec2 &size) -> void
		{
			bool flipImage = true;        //opengl is true
			ImGui::Image(texture ? texture->getHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
		}

		auto imageButton(const Quad2D *icon, const glm::vec2 &scale) -> bool
		{
			return false;
		}

		auto acceptFile(const std::function<void(const std::string &)> &callback) -> void
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("AssetFile", ImGuiDragDropFlags_None);
				if (data)
				{
					std::string file = (char *) data->Data;
					callback(file);
				}
				ImGui::EndDragDropTarget();
			}
		}

		auto combox(const std::string &id, const char *names[], int32_t namesLen, int32_t currentIndex) -> int32_t
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(id.c_str());
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			int32_t idx = -1;
			std::string id2  = "##" + id;
			if (ImGui::BeginCombo(id2.c_str(), names[currentIndex], 0))
			{
				for (auto n = 0; n < namesLen; n++)
				{
					if (ImGui::Selectable(names[n]))
					{
						idx = n;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			return idx;
		}
	};        // namespace ImGuiHelper
};            // namespace maple
