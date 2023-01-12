//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/Pipeline.h"
#include "VulkanHelper.h"

#include <functional>
#include <memory>

namespace maple
{
	class VulkanPipeline : public Pipeline
	{
	  public:
		constexpr static uint32_t MAX_DESCRIPTOR_SET = 1500;
		VulkanPipeline()                             = default;
		VulkanPipeline(const PipelineInfo &info);
		virtual ~VulkanPipeline();
		NO_COPYABLE(VulkanPipeline);

		auto init(const PipelineInfo &info) -> bool;

		auto getWidth() -> uint32_t override;
		auto getHeight() -> uint32_t override;

		auto bind(const CommandBuffer *commandBuffer, uint32_t layer = 0, int32_t cubeFace = -1, int32_t mipMapLevel = 0) -> FrameBuffer * override;
		auto bind(const CommandBuffer *commandBuffer, const glm::ivec4 &viewport) -> FrameBuffer * override;

		auto end(const CommandBuffer *commandBuffer) -> void override;
		auto clearRenderTargets(const CommandBuffer *commandBuffer) -> void override;

		inline auto getShader() const -> std::shared_ptr<Shader> override
		{
			return shader;
		};

		inline auto getPipelineLayout() const
		{
			return pipelineLayout;
		}

		virtual auto getPipelineBindPoint() const -> VkPipelineBindPoint
		{
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		}
		auto dispatchIndirect(const CommandBuffer *cmdBuffer, const StorageBuffer *ssbo) -> void override;

		auto drawIndexed(const CommandBuffer *cmdBuffer,
		                 uint32_t             indexCount,
		                 uint32_t             instanceCount,
		                 uint32_t             firstIndex,
		                 int32_t              vertexOffset,
		                 uint32_t             firstInstance) -> void override;

		auto bufferBarrier(const CommandBuffer *commandBuffer, const std::vector<std::shared_ptr<StorageBuffer>> &buffers, bool read) -> void override;

	  protected:
		std::shared_ptr<Shader> shader;
		VkPipelineLayout        pipelineLayout;
		VkPipeline              pipeline;

	  private:
		auto                                      transitionAttachments() -> void;
		auto                                      createFrameBuffers() -> void;
		std::shared_ptr<RenderPass>               renderPass;
		std::vector<std::shared_ptr<FrameBuffer>> framebuffers;
		bool                                      computePipline = false;
		bool                                      depthBiasEnabled;
		float                                     depthBiasConstant;
		float                                     depthBiasSlope;
	};
};        // namespace maple
