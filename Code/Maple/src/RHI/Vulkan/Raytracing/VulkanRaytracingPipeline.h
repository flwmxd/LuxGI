//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "RHI/Vulkan/VulkanHelper.h"
#include "RHI/Vulkan/VulkanPipeline.h"

#include <functional>
#include <memory>
#include <tuple>

namespace maple
{
	class ShaderBindingTable;
	class RayTracingProperties;

	class VulkanRaytracingPipeline : public VulkanPipeline
	{
	  public:
		constexpr static uint32_t MAX_DESCRIPTOR_SET = 1500;
		VulkanRaytracingPipeline(const PipelineInfo &info);
		NO_COPYABLE(VulkanRaytracingPipeline);

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

		inline auto getPipelineBindPoint() const -> VkPipelineBindPoint override
		{
			return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
		}

		inline auto getRayTracingProperties() const
		{
			return rayTracingProperties;
		}

		inline auto getShaderBindingTable() const
		{
			return sbt;
		}

		inline auto getSbtBuffer() const
		{
			return buffer;
		}

		auto traceRays(const CommandBuffer *commandBuffer, uint32_t width, uint32_t height, uint32_t depth) -> void override;

	  private:
		std::shared_ptr<ShaderBindingTable>   sbt;
		std::shared_ptr<RayTracingProperties> rayTracingProperties;
		std::shared_ptr<VulkanBuffer>         buffer;

		std::vector<VkShaderModule>                                             rayGens;
		std::vector<VkShaderModule>                                             missGens;
		std::vector<std::tuple<VkShaderModule, VkShaderModule, VkShaderModule>> hitsGen;
	};
};        // namespace maple
