//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanComputePipeline.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

#include "Engine/Vertex.h"
#include "Others/Console.h"

#include <memory>

#include "Application.h"

namespace maple
{
	VulkanComputePipeline::VulkanComputePipeline(const PipelineInfo &info)
	{
		init(info);
	}

	auto VulkanComputePipeline::init(const PipelineInfo &info) -> bool
	{
		PROFILE_FUNCTION();
		shader         = info.shader;
		auto vkShader  = std::static_pointer_cast<VulkanShader>(info.shader);
		description    = info;
		pipelineLayout = vkShader->getPipelineLayout();

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = pipelineLayout;
		computePipelineCreateInfo.flags  = 0;
		computePipelineCreateInfo.stage  = vkShader->getShaderStages()[0];
		VK_CHECK_RESULT(vkCreateComputePipelines(*VulkanDevice::get(),
		                                         VulkanDevice::get()->getPipelineCache(), 1,
		                                         &computePipelineCreateInfo, nullptr, &pipeline));

		VulkanHelper::setObjectName(info.pipelineName, (uint64_t) pipeline, VK_OBJECT_TYPE_PIPELINE);
		return true;
	}

	auto VulkanComputePipeline::bind(const CommandBuffer *cmdBuffer, uint32_t layer, int32_t cubeFace, int32_t mipMapLevel) -> FrameBuffer *
	{
		PROFILE_FUNCTION();
		vkCmdBindPipeline(static_cast<const VulkanCommandBuffer *>(cmdBuffer)->getCommandBuffer(),
		                  VK_PIPELINE_BIND_POINT_COMPUTE,
		                  pipeline);
		return nullptr;
	}
};        // namespace maple
