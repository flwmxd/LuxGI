//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "VulkanHelper.h"
#include "VulkanPipeline.h"

#include <functional>
#include <memory>

namespace maple
{
	class VulkanComputePipeline : public VulkanPipeline
	{
	  public:
		constexpr static uint32_t MAX_DESCRIPTOR_SET = 1500;
		VulkanComputePipeline(const PipelineInfo &info);
		NO_COPYABLE(VulkanComputePipeline);

		auto init(const PipelineInfo &info) -> bool;

		inline auto getWidth() -> uint32_t override
		{
			return 0;
		}
		inline auto getHeight() -> uint32_t override
		{
			return 0;
		}

		auto bind(const CommandBuffer *commandBuffer, uint32_t layer = 0, int32_t cubeFace = -1, int32_t mipMapLevel = 0) -> FrameBuffer * override;
		auto end(const CommandBuffer *commandBuffer) -> void override{};
		auto clearRenderTargets(const CommandBuffer *commandBuffer) -> void override{};

		auto getPipelineBindPoint() const -> VkPipelineBindPoint override
		{
			return VK_PIPELINE_BIND_POINT_COMPUTE;
		}
	};
};        // namespace maple
