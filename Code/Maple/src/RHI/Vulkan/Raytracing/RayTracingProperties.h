//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/Vulkan/Vk.h"

namespace maple
{
	class RayTracingProperties final
	{
	  public:
		RayTracingProperties();

		inline auto getMaxDescriptorSetAccelerationStructures() const
		{
			return accelProps.maxDescriptorSetAccelerationStructures;
		}
		inline auto getMaxGeometryCount() const
		{
			return accelProps.maxGeometryCount;
		}
		inline auto getMaxInstanceCount() const
		{
			return accelProps.maxInstanceCount;
		}
		inline auto getMaxPrimitiveCount() const
		{
			return accelProps.maxPrimitiveCount;
		}
		inline auto getMaxRayRecursionDepth() const
		{
			return pipelineProps.maxRayRecursionDepth;
		}
		inline auto getMaxShaderGroupStride() const
		{
			return pipelineProps.maxShaderGroupStride;
		}
		inline auto getMinAccelerationStructureScratchOffsetAlignment() const
		{
			return accelProps.minAccelerationStructureScratchOffsetAlignment;
		}
		inline auto getShaderGroupBaseAlignment() const
		{
			return pipelineProps.shaderGroupBaseAlignment;
		}
		inline auto getShaderGroupHandleCaptureReplaySize() const
		{
			return pipelineProps.shaderGroupHandleCaptureReplaySize;
		}
		inline auto getShaderGroupHandleSize() const
		{
			return pipelineProps.shaderGroupHandleSize;
		}

	  private:
		VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProps{};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR    pipelineProps{};
	};
}        // namespace maple