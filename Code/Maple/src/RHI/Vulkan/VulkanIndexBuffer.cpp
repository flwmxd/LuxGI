//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanIndexBuffer.h"
#include "Engine/Profiler.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
namespace maple
{
	VulkanIndexBuffer::VulkanIndexBuffer(const uint16_t *data, uint32_t initCount, BufferUsage bufferUsage, bool gpuOnly) :
	    VulkanBuffer(VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported() ?
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR :
                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                 initCount * sizeof(uint16_t), data, gpuOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU),
	    size(initCount * sizeof(uint16_t)),
	    count(initCount),
	    usage(bufferUsage)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t *data, uint32_t initCount, BufferUsage bufferUsage, bool gpuOnly) :
	    VulkanBuffer(
	        VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported() ?
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR :
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	        initCount * sizeof(uint32_t), data, gpuOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU),
	    size(initCount * sizeof(uint32_t)),
	    count(initCount),
	    usage(bufferUsage)
	{
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanIndexBuffer::bind(const CommandBuffer *commandBuffer) const -> void
	{
		vkCmdBindIndexBuffer(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	auto VulkanIndexBuffer::unbind() const -> void
	{
	}

	auto VulkanIndexBuffer::setData(uint32_t size, const void *data) -> void
	{
		setVkData(size, data);
	}

	auto VulkanIndexBuffer::releasePointer() -> void
	{
		PROFILE_FUNCTION();
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanIndexBuffer::getPointerInternal() -> void *
	{
		if (!mappedBuffer)
		{
			VulkanBuffer::map();
			mappedBuffer = true;
		}
		return mapped;
	}

	auto VulkanIndexBuffer::copy(CommandBuffer *cmd, GPUBuffer *to, const BufferCopy &copy) const -> void
	{
		auto         buffer = static_cast<VulkanIndexBuffer *>(to);
		VkBufferCopy bufferCopy;
		bufferCopy.dstOffset = copy.dstOffset;
		bufferCopy.srcOffset = copy.srcOffset;
		bufferCopy.size      = copy.size;
		vkCmdCopyBuffer(static_cast<VulkanCommandBuffer *>(cmd)->getCommandBuffer(), getVkBuffer(), buffer->getVkBuffer(), 1, &bufferCopy);
	}

};        // namespace maple
