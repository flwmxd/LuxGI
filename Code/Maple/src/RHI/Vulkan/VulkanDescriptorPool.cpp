//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanDescriptorPool.h"
#include "VulkanDevice.h"
#include "VulkanHelper.h"
namespace maple
{
	VulkanDescriptorPool::VulkanDescriptorPool(const DescriptorPoolCreateInfo &info)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;

		for (auto &v : info.poolInfos)
		{
			poolSizes.push_back({VkConverter::descriptorTypeToVK(v.type), v.size});
		}

		// Create info
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.poolSizeCount              = poolSizes.size();
		poolCreateInfo.pPoolSizes                 = poolSizes.data();
		poolCreateInfo.maxSets                    = info.maxSets;

		// Pool
		VK_CHECK_RESULT(vkCreateDescriptorPool(*VulkanDevice::get(), &poolCreateInfo, nullptr, &descriptorPool));
	}

	VulkanDescriptorPool::~VulkanDescriptorPool()
	{
		vkDestroyDescriptorPool(*VulkanDevice::get(), descriptorPool, nullptr);
	}

};        // namespace maple
