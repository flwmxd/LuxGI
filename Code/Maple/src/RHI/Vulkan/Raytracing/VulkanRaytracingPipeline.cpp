//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanRaytracingPipeline.h"

#include "RHI/Vulkan/VulkanBuffer.h"
#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "RHI/Vulkan/VulkanContext.h"
#include "RHI/Vulkan/VulkanDescriptorSet.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanFrameBuffer.h"
#include "RHI/Vulkan/VulkanRenderPass.h"
#include "RHI/Vulkan/VulkanShader.h"
#include "RHI/Vulkan/VulkanSwapChain.h"
#include "RHI/Vulkan/VulkanTexture.h"

#include "RayTracingProperties.h"
#include "ShaderBindingTable.h"

#include "Engine/Vertex.h"
#include "Others/Console.h"

#include <memory>

#include "Application.h"

namespace maple
{
#ifdef MAPLE_VULKAN
	namespace
	{
		inline auto alignedSize(uint32_t value, uint32_t alignment) -> uint32_t
		{
			return (value + alignment - 1) & ~(alignment - 1);
		}
	}        // namespace

	VulkanRaytracingPipeline::VulkanRaytracingPipeline(const PipelineInfo &info)
	{
		init(info);
	}

	auto VulkanRaytracingPipeline::init(const PipelineInfo &info) -> bool
	{
		PROFILE_FUNCTION();
		shader         = info.shader;
		auto vkShader  = std::static_pointer_cast<VulkanShader>(info.shader);
		description    = info;
		pipelineLayout = vkShader->getPipelineLayout();

		rayGens.clear();
		missGens.clear();
		hitsGen.clear();

		std::unordered_map<std::string, int32_t> map;

		for (auto &stage : vkShader->getShaderGroups())
		{
			if (auto iter = map.find(stage.first); iter == map.end())
			{
				map[stage.first] = map.size();
			}
		}

		hitsGen.resize(map.size());

		for (auto &stage : vkShader->getShaderGroups())
		{
			switch (stage.second.stage)
			{
				case VK_SHADER_STAGE_MISS_BIT_KHR:
					missGens.emplace_back(stage.second.module);
					break;
				case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
					rayGens.emplace_back(stage.second.module);
					break;
				case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: {
					std::get<0>(hitsGen[map[stage.first]]) = stage.second.module;
				}
				break;

				case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: {
					std::get<1>(hitsGen[map[stage.first]]) = stage.second.module;
				}
				break;

				case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: {
					std::get<2>(hitsGen[map[stage.first]]) = stage.second.module;
				}
				break;
			}
		}

		if (!rayGens.empty() || !missGens.empty() || !hitsGen.empty())
		{
			rayTracingProperties = std::make_shared<RayTracingProperties>();
			sbt                  = std::make_shared<ShaderBindingTable>(rayTracingProperties);
			sbt->addShader(rayGens, missGens, hitsGen);
		}

		VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
		pipelineCreateInfo.sType                             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineCreateInfo.pNext                             = nullptr;
		pipelineCreateInfo.flags                             = 0;

		pipelineCreateInfo.pStages    = sbt->getStages().data();
		pipelineCreateInfo.stageCount = sbt->getStages().size();
		pipelineCreateInfo.pGroups    = sbt->getGroups().data();
		pipelineCreateInfo.groupCount = sbt->getGroups().size();

		pipelineCreateInfo.maxPipelineRayRecursionDepth = info.maxRayRecursionDepth;
		pipelineCreateInfo.layout                       = pipelineLayout;
		pipelineCreateInfo.basePipelineHandle           = nullptr;
		pipelineCreateInfo.basePipelineIndex            = 0;

		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(*VulkanDevice::get(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

		VulkanHelper::setObjectName(info.pipelineName, (uint64_t) pipeline, VK_OBJECT_TYPE_PIPELINE);

		auto groupHandleSize = rayTracingProperties->getShaderGroupHandleSize();

		auto alignSize = alignedSize(groupHandleSize, rayTracingProperties->getShaderGroupBaseAlignment());

		const size_t groupCount = sbt->getGroups().size();
		size_t       sbtSize    = groupCount * alignSize;

		std::vector<uint8_t> mem(sbtSize);

		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(
		    *VulkanDevice::get(),
		    pipeline,
		    0,
		    static_cast<uint32_t>(groupCount),
		    sbtSize,
		    mem.data()));

		buffer = std::make_shared<VulkanBuffer>(
		    VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		    sbtSize,
		    nullptr,
		    VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT);

		buffer->map();
		auto dst = (uint8_t *) buffer->getMapped();
		auto src = mem.data();

		for (int32_t i = 0; i < groupCount; i++)
		{
			memcpy(dst, src, groupHandleSize);

			dst += alignSize;
			src += groupHandleSize;
		}

		return true;
	}

	auto VulkanRaytracingPipeline::bind(const CommandBuffer *cmdBuffer, uint32_t layer, int32_t cubeFace, int32_t mipMapLevel) -> FrameBuffer *
	{
		PROFILE_FUNCTION();
		vkCmdBindPipeline(static_cast<const VulkanCommandBuffer *>(cmdBuffer)->getCommandBuffer(),
		                  VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		                  pipeline);
		return nullptr;
	}

	auto VulkanRaytracingPipeline::traceRays(const CommandBuffer *commandBuffer, uint32_t width, uint32_t height, uint32_t depth) -> void
	{
		VkDeviceSize groupSize   = alignedSize(rayTracingProperties->getShaderGroupHandleSize(), rayTracingProperties->getShaderGroupBaseAlignment());
		VkDeviceSize groupStride = groupSize;
		//TODO ....
		const VkStridedDeviceAddressRegionKHR raygenSbt   = {buffer->getDeviceAddress(), groupStride, groupSize};
		const VkStridedDeviceAddressRegionKHR missSbt     = {buffer->getDeviceAddress() + sbt->getMissGroupOffset(), groupStride, groupSize * missGens.size()};
		const VkStridedDeviceAddressRegionKHR hitSbt      = {buffer->getDeviceAddress() + sbt->getHitGroupOffset(), groupStride, groupSize * hitsGen.size()};
		const VkStridedDeviceAddressRegionKHR callableSbt = {0, 0, 0};

		vkCmdTraceRaysKHR(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
		                  &raygenSbt, &missSbt, &hitSbt, &callableSbt, width, height, depth);
	}
#endif
};        // namespace maple
