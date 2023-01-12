//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <vulkan/vulkan.h>
#ifdef USE_VMA_ALLOCATOR
#	include <vk_mem_alloc.h>
#endif        // USE_VMA_ALLOCATOR

namespace maple
{
	struct VkConfig
	{
		static constexpr bool StandardValidationLayer = false;
		static constexpr bool AssistanceLayer         = false;
#ifdef _DEBUG
		static constexpr bool EnableValidationLayers = true;
#else
		static constexpr bool EnableValidationLayers = false;
#endif
	};

	class VulkanDevice;
	class VulkanBuffer;
	class VulkanImage;
	class VulkanImageView;
	class VulkanInstance;
	class VulkanQueryPool;
	class VulkanSurface;
	class VulkanSwapChain;
	class VulkanDescriptorPool;
	class VulkanFence;
	class VulkanRenderPass;
	class VulkanFrameBuffer;
	class VulkanShader;
	class VulkanCommandPool;
	class VulkanCommandBuffer;

	auto loadVKRayTracingPipelineKHR(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) -> int32_t;
	auto loadVKAccelerationStructureKHR(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr) -> int32_t;
}        // namespace maple
