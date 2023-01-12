//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Engine/Timestep.h"
namespace maple
{
	class CommandBuffer;

	class MAPLE_EXPORT ImGuiRenderer
	{
	  public:
		static auto create(uint32_t width, uint32_t height, bool clearScreen) -> std::shared_ptr<ImGuiRenderer>;

		virtual ~ImGuiRenderer()                                       = default;
		virtual auto init() -> void                                    = 0;
		virtual auto newFrame(const Timestep &dt) -> void              = 0;
		virtual auto render(CommandBuffer *commandBuffer) -> void      = 0;
		virtual auto onResize(uint32_t width, uint32_t height) -> void = 0;
		virtual auto rebuildFontTexture() -> void                      = 0;
		virtual auto clear() -> void
		{}
	};
}        // namespace maple
