//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DescriptorPool.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanDescriptorPool.h"
#endif        // MAPLE_VULKAN

namespace maple
{
	auto DescriptorPool::create(const DescriptorPoolCreateInfo &desc) -> Ptr
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanDescriptorPool>(desc);
#endif
		return nullptr;
	}
}        // namespace maple
