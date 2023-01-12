//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AccelerationStructure.h"
#include "StorageBuffer.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/Vk.h"
#	include "RHI/Vulkan/VulkanContext.h"
#	include "RHI/Vulkan/Raytracing/RayTracingProperties.h"
#	include "RHI/Vulkan/Raytracing/VulkanAccelerationStructure.h"
#	include "RHI/Vulkan/VulkanBuffer.h"
#	include "RHI/Vulkan/VulkanVertexBuffer.h"
#	include "RHI/Vulkan/VulkanIndexBuffer.h"
#else
#endif        // MAPLE_VULKAN

namespace maple
{
	auto AccelerationStructure::createTopLevel(const uint32_t maxInstanceCount) -> Ptr
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanAccelerationStructure>(maxInstanceCount);
#else
		return std::make_shared<NullAccelerationStructure>();
#endif        // MAPLE_VULKAN
	}

	auto AccelerationStructure::createBottomLevel(const VertexBuffer::Ptr &vertexBuffer, const IndexBuffer::Ptr &indexBuffer, uint32_t vertexCount, BatchTask::Ptr batchTask) -> Ptr
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanAccelerationStructure>(vertexBuffer->getAddress(), indexBuffer->getAddress(), vertexCount, indexBuffer->getCount(), batchTask);
#else
		return std::make_shared<NullAccelerationStructure>();
#endif
	}

}        // namespace maple