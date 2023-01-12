//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RayTracingProperties.h"
#include "RHI/Vulkan/VulkanDevice.h"

namespace maple
{
	RayTracingProperties::RayTracingProperties()
	{
		accelProps.sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		pipelineProps.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		pipelineProps.pNext               = &accelProps;
		VkPhysicalDeviceProperties2 props = {};
		props.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		props.pNext                       = &pipelineProps;
		vkGetPhysicalDeviceProperties2(*VulkanDevice::get()->getPhysicalDevice(), &props);
	}
};        // namespace maple