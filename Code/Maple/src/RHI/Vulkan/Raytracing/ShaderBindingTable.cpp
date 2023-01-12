//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ShaderBindingTable.h"
#include "Others/Console.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RayTracingProperties.h"

namespace maple
{
	namespace
	{
		inline auto alignedSize(uint32_t value, uint32_t alignment) -> uint32_t
		{
			return (value + alignment - 1) & ~(alignment - 1);
		}
	}        // namespace

	ShaderBindingTable::ShaderBindingTable(const std::shared_ptr<RayTracingProperties> &rayTracingProperties) :
	    rayTracingProperties(rayTracingProperties)
	{
	}

	auto ShaderBindingTable::addShader(const std::vector<VkShaderModule> &                                            rayGens,
	                                   const std::vector<VkShaderModule> &                                            missGens,
	                                   const std::vector<std::tuple<VkShaderModule, VkShaderModule, VkShaderModule>> &hits) -> void
	{
		auto alignSize = alignedSize(rayTracingProperties->getShaderGroupHandleSize(), rayTracingProperties->getShaderGroupBaseAlignment());

		for (auto shaderModule : rayGens)
		{
			int32_t id    = stages.size();
			auto &  stage = stages.emplace_back();
			stage.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.module  = shaderModule;
			stage.stage   = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			stage.pName   = "main";

			VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
			groupInfo.sType                                = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupInfo.pNext                                = nullptr;
			groupInfo.type                                 = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groupInfo.generalShader                        = id;
			groupInfo.closestHitShader                     = VK_SHADER_UNUSED_KHR;
			groupInfo.anyHitShader                         = VK_SHADER_UNUSED_KHR;
			groupInfo.intersectionShader                   = VK_SHADER_UNUSED_KHR;
			groups.emplace_back(groupInfo);
			rayGenSize += alignSize;
		}

		for (auto shaderModule : missGens)
		{
			int32_t id    = stages.size();
			auto &  stage = stages.emplace_back();
			stage.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.module  = shaderModule;
			stage.stage   = VK_SHADER_STAGE_MISS_BIT_KHR;
			stage.pName   = "main";

			VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
			groupInfo.sType                                = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupInfo.pNext                                = nullptr;
			groupInfo.type                                 = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groupInfo.generalShader                        = id;
			groupInfo.closestHitShader                     = VK_SHADER_UNUSED_KHR;
			groupInfo.anyHitShader                         = VK_SHADER_UNUSED_KHR;
			groupInfo.intersectionShader                   = VK_SHADER_UNUSED_KHR;
			groups.emplace_back(groupInfo);
			missGroupSize += alignSize;
		}

		for (auto [cloestHit, anyHit, intersection] : hits)
		{
			VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};

			groupInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupInfo.pNext              = nullptr;
			groupInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			groupInfo.generalShader      = VK_SHADER_UNUSED_KHR;
			groupInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
			groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

			if (cloestHit != nullptr)
			{
				int32_t id    = stages.size();
				auto &  stage = stages.emplace_back();

				stage.sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.module               = cloestHit;
				stage.stage                = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				stage.pName                = "main";
				groupInfo.closestHitShader = id;
			}

			if (anyHit != nullptr)
			{
				int32_t id    = stages.size();
				auto &  stage = stages.emplace_back();

				stage.sType            = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.module           = anyHit;
				stage.stage            = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				stage.pName            = "main";
				groupInfo.anyHitShader = id;
			}

			if (intersection != nullptr)
			{
				int32_t id    = stages.size();
				auto &  stage = stages.emplace_back();

				stage.sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stage.module                 = intersection;
				stage.stage                  = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				stage.pName                  = "main";
				groupInfo.intersectionShader = id;
			}

			groups.emplace_back(groupInfo);

			hitGroupSize += alignSize;
		}
	}
}        // namespace maple
