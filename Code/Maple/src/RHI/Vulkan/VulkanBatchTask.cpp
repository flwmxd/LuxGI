//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanBatchTask.h"
#include "RHI/Vulkan/Raytracing/VulkanAccelerationStructure.h"
#include <cmath>

namespace maple
{
#ifdef MAPLE_VULKAN
	auto VulkanBatchTask::execute() -> void
	{
		auto cmd = VulkanHelper::beginSingleTimeCommands();

		std::shared_ptr<VulkanBuffer> scratchBuffer;

		if (requests.size() > 0)
		{
			VkMemoryBarrier memoryBarrier;
			memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.pNext         = nullptr;
			memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

			VkDeviceSize scratchBufferSize = 0;

			for (auto &request : requests)
				scratchBufferSize = std::max(scratchBufferSize, request.accelerationStructure->getBuildSizes().buildScratchSize);

			scratchBuffer = std::make_shared<VulkanBuffer>(
			    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			    scratchBufferSize, nullptr, VMA_MEMORY_USAGE_GPU_ONLY, 0);

			for (auto &request : requests)
			{
				const VkAccelerationStructureBuildRangeInfoKHR *buildRanges = &request.buildRanges[0];

				VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};

				buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
				buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
				buildInfo.flags                     = request.accelerationStructure->getFlags();
				buildInfo.srcAccelerationStructure  = VK_NULL_HANDLE;
				buildInfo.dstAccelerationStructure  = request.accelerationStructure->getAccelerationStructure();
				buildInfo.geometryCount             = (uint32_t) request.geometries.size();
				buildInfo.pGeometries               = request.geometries.data();
				buildInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();

				vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &buildRanges);

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memoryBarrier, 0, 0, 0, 0);
			}
		}

		VulkanHelper::endSingleTimeCommands(cmd);
	}

	auto VulkanBatchTask::buildBlas(VulkanAccelerationStructure *                               accelerationStructure,
	                                const std::vector<VkAccelerationStructureGeometryKHR> &     geometries,
	                                const std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges) -> void
	{
		if (geometries.size() > 0 || buildRanges.size() > 0)
			requests.push_back({accelerationStructure, geometries, buildRanges});
		else
		{
			throw std::runtime_error("(Vulkan) Building a BLAS fail.");
		}
	}
#endif        // MAPLE_VULKAN
}        // namespace maple