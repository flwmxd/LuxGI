//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanFence.h"
#include "Others/Console.h"
#include "VulkanDevice.h"

namespace maple
{
	VulkanFence::VulkanFence(bool createSignaled) :
	    signaled(createSignaled)
	{
		PROFILE_FUNCTION();

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		vkCreateFence(*VulkanDevice::get(), &fenceCreateInfo, nullptr, &handle);
	}

	VulkanFence::~VulkanFence()
	{
		vkDestroyFence(*VulkanDevice::get(), handle, nullptr);
	}

	auto VulkanFence::checkState() -> bool
	{
		MAPLE_ASSERT(!signaled, "Fence Signaled");

		const VkResult result = vkGetFenceStatus(*VulkanDevice::get(), handle);
		if (result == VK_SUCCESS)
		{
			signaled = true;
			return true;
		}

		return false;
	}
	auto VulkanFence::isSignaled() -> bool
	{
		PROFILE_FUNCTION();
		if (signaled)
		{
			return true;
		}
		else
		{
			return checkState();
		}
	}
	auto VulkanFence::wait() -> bool
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(!signaled, "Fence Signaled");

		const VkResult result = vkWaitForFences(*VulkanDevice::get(), 1, &handle, true, UINT32_MAX);

		VK_CHECK_RESULT(result);
		if (result == VK_SUCCESS)
		{
			signaled = true;
			return false;
		}
		return true;
	}

	auto VulkanFence::reset() -> void
	{
		PROFILE_FUNCTION();
		if (signaled)
			VK_CHECK_RESULT(vkResetFences(*VulkanDevice::get(), 1, &handle));
		signaled = false;
	}

	auto VulkanFence::waitAndReset() -> void
	{
		PROFILE_FUNCTION();
		if (!isSignaled())
			wait();
		reset();
	}
};        // namespace maple
