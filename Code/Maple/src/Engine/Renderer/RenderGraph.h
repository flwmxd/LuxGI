//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "Engine/Timestep.h"
#include "Renderer.h"
#include <memory>
#include <vector>

namespace maple
{
	class Camera;
	class Scene;
	class GBuffer;
	class Texture;
	class PrefilterRenderer;

	namespace component
	{
		struct Light;
		class Transform;
	};        // namespace component

	class MAPLE_EXPORT RenderGraph
	{
	  public:
		RenderGraph()  = default;
		~RenderGraph() = default;
		auto init(uint32_t width, uint32_t height) -> void;
		auto onResize(uint32_t width, uint32_t height) -> void;
		auto beginScene(Scene *scene) -> void;

		auto onUpdate(const Timestep &timeStep, Scene *scene) -> void;
		auto onImGui() -> void;

		auto setRenderTarget(Scene *scene, const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer = true) -> void;

		inline auto getGBuffer() const
		{
			return gBuffer;
		}

		inline auto setScreenBufferSize(uint32_t width, uint32_t height)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;
			screenBufferWidth  = width;
			screenBufferHeight = height;
		}

		auto pingPong() -> void;

	  private:
		std::vector<std::function<void(const CommandBuffer*)>> tasks;

		bool previewFocused = false;

		std::shared_ptr<GBuffer> gBuffer;

		uint32_t screenBufferWidth  = 0;
		uint32_t screenBufferHeight = 0;
	};
};        // namespace maple
