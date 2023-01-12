//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "BufferLayout.h"
#include "Definitions.h"
#include "Shader.h"
#include <array>
#include <memory>

namespace maple
{
	namespace capture_graph
	{
		namespace component
		{
			struct RenderGraph;
		};
	}        // namespace capture_graph

	class ShaderBindingTable;

	class DescriptorSet;

	class MAPLE_EXPORT Pipeline
	{
	  public:
		using Ptr = std::shared_ptr<Pipeline>;

		static auto get(const PipelineInfo &pipelineDesc) -> std::shared_ptr<Pipeline>;
		static auto get(const PipelineInfo &pipelineDesc, const std::vector<std::shared_ptr<DescriptorSet>> &sets, capture_graph::component::RenderGraph &) -> std::shared_ptr<Pipeline>;

		virtual ~Pipeline() = default;

		virtual auto getWidth() -> uint32_t                                                                                                        = 0;
		virtual auto getHeight() -> uint32_t                                                                                                       = 0;
		virtual auto getShader() const -> std::shared_ptr<Shader>                                                                                  = 0;
		virtual auto bind(const CommandBuffer *commandBuffer, uint32_t layer = 0, int32_t cubeFace = -1, int32_t mipMapLevel = 0) -> FrameBuffer * = 0;
		virtual auto bind(const CommandBuffer *commandBuffer, const glm::ivec4 &viewport) -> FrameBuffer *                      = 0;

		virtual auto end(const CommandBuffer *commandBuffer) -> void = 0;
		virtual auto clearRenderTargets(const CommandBuffer *commandBuffer) -> void{};
		virtual auto traceRays(const CommandBuffer *commandBuffer, uint32_t width, uint32_t height, uint32_t depth) -> void{};
		virtual auto dispatchIndirect(const CommandBuffer *cmdBuffer, const StorageBuffer *ssbo) -> void{};

		virtual auto drawIndexed(const CommandBuffer *cmdBuffer,
		                         uint32_t             indexCount,
		                         uint32_t             instanceCount,
		                         uint32_t             firstIndex,
		                         int32_t              vertexOffset,
		                         uint32_t             firstInstance) -> void = 0;

		virtual auto bufferBarrier(const CommandBuffer *                              commandBuffer,
		                           const std::vector<std::shared_ptr<StorageBuffer>> &buffers, bool read) -> void = 0;

	  protected:
		PipelineInfo description;
	};
}        // namespace maple
