//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/Vulkan/VulkanHelper.h"

#include <memory>
#include <tuple>

namespace maple
{
	class RayTracingProperties;

	class ShaderBindingTable final
	{
	  public:
		using Ptr = std::shared_ptr<ShaderBindingTable>;

		ShaderBindingTable(const std::shared_ptr<RayTracingProperties> &rayTracingProperties);

		auto addShader(
		    const std::vector<VkShaderModule> &                                            rayGens,
		    const std::vector<VkShaderModule> &                                            missGens,
		    const std::vector<std::tuple<VkShaderModule, VkShaderModule, VkShaderModule>> &hits) -> void;

		inline auto &getStages() const
		{
			return stages;
		}
		inline auto &getGroups() const
		{
			return groups;
		}

		inline auto getHitGroupOffset() const
		{
			return rayGenSize + missGroupSize;
		}
		inline auto getMissGroupOffset() const
		{
			return rayGenSize;
		}

	  private:
		std::shared_ptr<RayTracingProperties> rayTracingProperties;

		std::vector<VkPipelineShaderStageCreateInfo>      stages;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

		VkDeviceSize rayGenSize    = 0;
		VkDeviceSize hitGroupSize  = 0;
		VkDeviceSize missGroupSize = 0;
	};
}        // namespace maple