//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanCommandPool.h"
#include "Others/Console.h"
#include "VulkanDevice.h"

namespace maple
{
	VulkanCommandPool::VulkanCommandPool(int32_t queueIndex, VkCommandPoolCreateFlags flags)
	{
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = queueIndex;
		createInfo.flags            = flags;
		VK_CHECK_RESULT(vkCreateCommandPool(*VulkanDevice::get(), &createInfo, nullptr, &commandPool));
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		vkDestroyCommandPool(*VulkanDevice::get(), commandPool, nullptr);
	}

	auto VulkanCommandPool::reset() -> void
	{
		vkResetCommandPool(*VulkanDevice::get(), commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	}
};        // namespace maple
