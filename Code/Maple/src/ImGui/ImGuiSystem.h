//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Timestep.h"
#include "Event/EventHandler.h"
#include <memory>

namespace maple
{
	class ImGuiRenderer;
	class Scene;

	class ImGuiSystem
	{
	  public:
		ImGuiSystem(bool clearScreen = false);
		~ImGuiSystem();

		auto newFrame(const Timestep &step) -> void;
		auto onInit() -> void;

		auto onRender(Scene *scene) -> void;
		auto addIcon() -> void;
		auto onResize(uint32_t w, uint32_t h) -> void;
		auto setTheme() -> void;
		auto update() -> void;

	  private:
		bool                           clearScreen = false;
		std::shared_ptr<ImGuiRenderer> imguiRender;
		EventHandler                   handler;
	};
}        // namespace maple