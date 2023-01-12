//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanAccelerationStructure.h"
#include "Engine/Vertex.h"
#include "Others/Console.h"
#include "RHI/Vulkan/VulkanBatchTask.h"
#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RayTracingProperties.h"

namespace maple
{
#ifdef MAPLE_VULKAN
	VulkanAccelerationStructure::VulkanAccelerationStructure(const uint32_t maxInstanceCount)
	{
		instanceBufferDevice = std::make_shared<VulkanBuffer>(
		    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		    sizeof(VkAccelerationStructureInstanceKHR) * maxInstanceCount, nullptr, VMA_MEMORY_USAGE_GPU_ONLY, 0);

		instanceBufferHost = std::make_shared<VulkanBuffer>(
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    sizeof(VkAccelerationStructureInstanceKHR) * maxInstanceCount, nullptr, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT);

		VkDeviceOrHostAddressConstKHR instanceDeviceAddress{};
		instanceDeviceAddress.deviceAddress = instanceBufferDevice->getDeviceAddress();

		VkAccelerationStructureGeometryKHR tlasGeometry{};
		tlasGeometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		tlasGeometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		tlasGeometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		tlasGeometry.geometry.instances.data            = instanceDeviceAddress;

		// Create TLAS
		Desc desc;
		create(desc.setGeometryCount(1)
		           .setGeometries({tlasGeometry})
		           .setMaxPrimitiveCounts({maxInstanceCount})
		           .setType(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
		           .setFlags(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR));

		scratchBuffer = std::make_shared<VulkanBuffer>(
		    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		    getBuildScratchSize(), nullptr, VMA_MEMORY_USAGE_GPU_ONLY, 0);
	}

	VulkanAccelerationStructure::VulkanAccelerationStructure(uint64_t vertexAddress, uint64_t indexAddress,
	                                                         uint32_t vertexCount, uint32_t indexCount,
	                                                         std::shared_ptr<BatchTask> batch, bool opaque)
	{
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRanges;
		std::vector<VkAccelerationStructureGeometryKHR>       geometries;
		std::vector<uint32_t>                                 maxPrimitiveCounts;

		auto vkBatch = std::static_pointer_cast<VulkanBatchTask>(batch);

		for (auto i = 0; i < 1; i++)        //TODO With subMeshes.
		{
			VkAccelerationStructureGeometryKHR geometry{};
			VkGeometryFlagsKHR                 geometryFlags = 0;

			if (opaque)
				geometryFlags = VK_GEOMETRY_OPAQUE_BIT_KHR;

			geometry.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.pNext                                       = nullptr;
			geometry.geometryType                                = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geometry.geometry.triangles.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			geometry.geometry.triangles.pNext                    = nullptr;
			geometry.geometry.triangles.vertexData.deviceAddress = vertexAddress;
			geometry.geometry.triangles.vertexStride             = sizeof(maple::Vertex);
			geometry.geometry.triangles.maxVertex                = vertexCount;
			geometry.geometry.triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
			geometry.geometry.triangles.indexData.deviceAddress  = indexAddress;
			geometry.geometry.triangles.indexType                = VK_INDEX_TYPE_UINT32;
			geometry.flags                                       = geometryFlags;

			geometries.push_back(geometry);
			maxPrimitiveCounts.push_back(indexCount / 3);

			VkAccelerationStructureBuildRangeInfoKHR buildRange{};
			buildRange.primitiveCount  = indexCount / 3;
			buildRange.primitiveOffset = 0;
			buildRange.firstVertex     = 0;
			buildRange.transformOffset = 0;

			buildRanges.push_back(buildRange);
		}

		// Create BLAS
		Desc desc;
		create(desc
		           .setGeometryCount(geometries.size())
		           .setGeometries(geometries)
		           .setMaxPrimitiveCounts(maxPrimitiveCounts)
		           .setType(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
		           .setFlags(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR));

		vkBatch->buildBlas(this, geometries, buildRanges);
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		if (accelerationStructure != nullptr)
		{
			vkDestroyAccelerationStructureKHR(*VulkanDevice::get(), accelerationStructure, nullptr);
			accelerationStructure = nullptr;
		}
	}

	auto VulkanAccelerationStructure::updateTLAS(const glm::mat4 &transform, uint32_t instanceId, uint64_t instanceAddress) -> uint64_t
	{
		VkAccelerationStructureInstanceKHR *geometryBuffer = (VkAccelerationStructureInstanceKHR *) instanceBufferHost->getMapped();
		if (geometryBuffer == nullptr)
		{
			geometryBuffer = (VkAccelerationStructureInstanceKHR *) mapHost();
		}

		VkAccelerationStructureInstanceKHR &vkASInstance = geometryBuffer[instanceId];

		vkASInstance.instanceCustomIndex                    = instanceId;
		vkASInstance.mask                                   = 0xFF;
		vkASInstance.instanceShaderBindingTableRecordOffset = 0;
		vkASInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		vkASInstance.accelerationStructureReference         = instanceAddress;
		auto trans                                          = glm::transpose(transform);
		std::memcpy(&vkASInstance.transform, &trans, sizeof(vkASInstance.transform));

		return instanceId * sizeof(VkAccelerationStructureInstanceKHR);
	}

	auto VulkanAccelerationStructure::create(const Desc &desc) -> void
	{
		buildGeometryInfo = desc.buildGeometryInfo;
		flags             = desc.buildGeometryInfo.flags;

		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
		    *VulkanDevice::get(),
		    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		    &buildGeometryInfo,
		    desc.maxPrimitiveCounts.data(),
		    &buildSizesInfo);

		buffer = std::make_shared<VulkanBuffer>(
		    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		    buildSizesInfo.accelerationStructureSize, nullptr, VMA_MEMORY_USAGE_GPU_ONLY, 0);

		auto createInfo = desc.createInfo;

		createInfo.buffer = buffer->getVkBuffer();
		createInfo.size   = buildSizesInfo.accelerationStructureSize;

		VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(*VulkanDevice::get(), &createInfo, nullptr, &accelerationStructure));

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};

		addressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = accelerationStructure;

		deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(*VulkanDevice::get(), &addressInfo);

		if (deviceAddress == 0)
		{
			throw std::runtime_error("(Vulkan) Failed to create Acceleration Structure.");
		}
	}

	VulkanAccelerationStructure::Desc::Desc()
	{
		createInfo       = {};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;

		buildGeometryInfo       = {};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.pNext = nullptr;
	}

	auto VulkanAccelerationStructure::Desc::setType(VkAccelerationStructureTypeKHR type) -> Desc &
	{
		createInfo.type        = type;
		buildGeometryInfo.type = type;
		return *this;
	}

	auto VulkanAccelerationStructure::Desc::setGeometries(const std::vector<VkAccelerationStructureGeometryKHR> &geometrys) -> Desc &
	{
		geometries                    = geometrys;
		buildGeometryInfo.pGeometries = geometries.data();
		return *this;
	}

	auto VulkanAccelerationStructure::Desc::setMaxPrimitiveCounts(const std::vector<uint32_t> &primitiveCounts) -> Desc &
	{
		maxPrimitiveCounts = primitiveCounts;
		return *this;
	}

	auto VulkanAccelerationStructure::Desc::setGeometryCount(uint32_t count) -> Desc &
	{
		buildGeometryInfo.geometryCount = count;
		return *this;
	}

	auto VulkanAccelerationStructure::Desc::setFlags(VkBuildAccelerationStructureFlagsKHR flags) -> Desc &
	{
		buildGeometryInfo.flags = flags;
		return *this;
	}

	auto VulkanAccelerationStructure::Desc::setDeviceAddress(VkDeviceAddress address) -> Desc &
	{
		createInfo.deviceAddress = address;
		return *this;
	}

	auto VulkanAccelerationStructure::mapHost() -> void *
	{
		instanceBufferHost->map();
		return instanceBufferHost->getMapped();
	}

	auto VulkanAccelerationStructure::unmap() -> void
	{
		return instanceBufferHost->unmap();
	}

	auto VulkanAccelerationStructure::copyToGPU(const CommandBuffer *cmd, uint32_t instanceSize, uint64_t offset) -> void
	{
		if (instanceBufferHost != nullptr && instanceBufferDevice != nullptr && instanceSize > 0)
		{
		
			VkBufferCopy copyRegion{};

			copyRegion.srcOffset = offset;
			copyRegion.dstOffset = offset;
			copyRegion.size      = sizeof(VkAccelerationStructureInstanceKHR) * instanceSize;

			auto vkCmd = static_cast<const VulkanCommandBuffer *>(cmd);

			vkCmdCopyBuffer(vkCmd->getCommandBuffer(), instanceBufferHost->getVkBuffer(), instanceBufferDevice->getVkBuffer(), 1, &copyRegion);
		}
	}

	auto VulkanAccelerationStructure::build(const CommandBuffer *cmd, uint32_t instanceSize, uint32_t instanceOffset) -> void
	{
		if (instanceBufferDevice)
		{
			if (instanceBufferDevice->getMapped() != nullptr)
			{
				instanceBufferDevice->unmap();
			}

			auto vkCmd = static_cast<const VulkanCommandBuffer *>(cmd);
			{
				VkMemoryBarrier memoryBarrier;
				memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				memoryBarrier.pNext         = nullptr;
				memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				vkCmdPipelineBarrier(vkCmd->getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
			}

			VkAccelerationStructureGeometryKHR geometry{};
			geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
			geometry.geometry.instances.data.deviceAddress = instanceBufferDevice->getDeviceAddress();

			VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};

			buildInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			buildInfo.flags                     = flags;
			buildInfo.srcAccelerationStructure  = built ? accelerationStructure : VK_NULL_HANDLE;
			buildInfo.dstAccelerationStructure  = accelerationStructure;
			buildInfo.geometryCount             = 1;
			buildInfo.pGeometries               = &geometry;
			buildInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();

			VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;

			buildRangeInfo.primitiveCount  = instanceSize;
			buildRangeInfo.primitiveOffset = instanceOffset;
			buildRangeInfo.firstVertex     = 0;
			buildRangeInfo.transformOffset = 0;

			const VkAccelerationStructureBuildRangeInfoKHR *ptrBuildRangeInfo = &buildRangeInfo;

			vkCmdBuildAccelerationStructuresKHR(vkCmd->getCommandBuffer(), 1, &buildInfo, &ptrBuildRangeInfo);

			{
				VkMemoryBarrier memoryBarrier;
				memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				memoryBarrier.pNext         = nullptr;
				memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				vkCmdPipelineBarrier(vkCmd->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memoryBarrier, 0, 0, 0, 0);
			}

			built = true;
		}
	}
#endif
}        // namespace maple