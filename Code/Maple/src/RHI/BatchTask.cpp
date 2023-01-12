//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "BatchTask.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanBatchTask.h"
#endif        // MAPLE_VULKAN

namespace maple
{
	auto BatchTask::create() -> Ptr
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanBatchTask>();
#endif
		return nullptr;
	}
}        // namespace maple