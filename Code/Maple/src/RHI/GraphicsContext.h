//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Others/Console.h"
#include <memory>

namespace maple
{
	enum class GraphicsAPI : uint32_t
	{
		OPENGL = 0,
		VULKAN,
		DIRECT3D,
		METAL,
		NONE,
	};

	template <typename T>
	struct CacheAsset
	{
		CacheAsset(std::shared_ptr<T> asset, uint64_t lastTimestamp) :
		    asset(asset),
		    lastTimestamp(lastTimestamp){};
		std::shared_ptr<T> asset;
		uint64_t           lastTimestamp;
	};

	class MAPLE_EXPORT CommandBuffer;
	class MAPLE_EXPORT SwapChain;
	class MAPLE_EXPORT Pipeline;
	class MAPLE_EXPORT FrameBuffer;
	class MAPLE_EXPORT Shader;

	class MAPLE_EXPORT GraphicsContext
	{
	  public:
		using Ptr = std::shared_ptr<GraphicsContext>;

		virtual ~GraphicsContext() = default;

		inline static auto getGraphicsAPI()
		{
#if defined(MAPLE_OPENGL)
			return GraphicsAPI ::OPENGL;
#elif defined(MAPLE_VULKAN)
			return GraphicsAPI ::VULKAN;
#endif        // MAPLE_OPENGL
			return GraphicsAPI ::NONE;
		}

		virtual auto init() -> void                                       = 0;
		virtual auto present() -> void                                    = 0;
		virtual auto getMinUniformBufferOffsetAlignment() const -> size_t = 0;
		virtual auto alignedDynamicUboSize(size_t size) const -> size_t   = 0;
		virtual auto waitIdle() const -> void                             = 0;
		virtual auto onImGui() -> void                                    = 0;
		virtual auto getGPUMemoryUsed() -> float                          = 0;
		virtual auto getTotalGPUMemory() -> float                         = 0;
		virtual auto isRaytracingSupported() -> bool                      = 0;
		virtual auto immediateSubmit(const std::function<void(CommandBuffer *)> &execute) -> void = 0;

		inline auto getSwapChain() -> std::shared_ptr<SwapChain>
		{
			MAPLE_ASSERT(swapChain != nullptr, "SwapChain must be initialized");
			return swapChain;
		}

		static auto create() -> std::shared_ptr<GraphicsContext>;

		inline auto &getPipelineCache()
		{
			return pipelineCache;
		}

		inline auto &getFrameBufferCache()
		{
			return frameBufferCache;
		}

		auto clearUnused() -> void;

	  protected:
		std::shared_ptr<SwapChain>                               swapChain;
		std::unordered_map<std::size_t, CacheAsset<Pipeline>>    pipelineCache;
		std::unordered_map<std::size_t, CacheAsset<FrameBuffer>> frameBufferCache;
	};

	namespace global::component
	{
		struct GraphicsContext
		{
			maple::GraphicsContext::Ptr context;
		};
	}        // namespace global::component
}        // namespace maple
