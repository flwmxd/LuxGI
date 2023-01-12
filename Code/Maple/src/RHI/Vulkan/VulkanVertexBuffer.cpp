//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanVertexBuffer.h"
#include "Engine/Profiler.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"

namespace maple
{
	VulkanVertexBuffer::VulkanVertexBuffer(const BufferUsage &usage)
	{
		auto flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		if (VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported())
		{
			flags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		}
		VulkanBuffer::setUsage(flags);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const void *data, uint32_t size, bool gpuOnly) :
	    VulkanBuffer(VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported() ?
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR :
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                 size, data, gpuOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU)
	{
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		releasePointer();
	}

	auto VulkanVertexBuffer::resize(uint32_t size) -> void
	{
		PROFILE_FUNCTION();
		if (this->size != size)
		{
			VulkanBuffer::resize(size, nullptr);
		}
	}

	auto VulkanVertexBuffer::setData(uint32_t size, const void *data) -> void
	{
		PROFILE_FUNCTION();
		if (size != this->size)
		{
			VulkanBuffer::resize(size, data);
		}
		else
		{
			VulkanBuffer::setVkData(size, data);
		}
	}

	auto VulkanVertexBuffer::setDataSub(uint32_t size, const void *data, uint32_t offset) -> void
	{
		PROFILE_FUNCTION();
		if (size != this->size)
		{
			VulkanBuffer::resize(size, data);
		}
		else
		{
			VulkanBuffer::setVkData(size, data);
		}
	}

	auto VulkanVertexBuffer::releasePointer() -> void
	{
		PROFILE_FUNCTION();
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanVertexBuffer::bind(const CommandBuffer *commandBuffer, Pipeline *pipeline) -> void
	{
		PROFILE_FUNCTION();
		VkDeviceSize offsets[1] = {0};
		if (commandBuffer)
			vkCmdBindVertexBuffers(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), 0, 1, &buffer, offsets);
	}

	auto VulkanVertexBuffer::unbind() -> void
	{
	}

	auto VulkanVertexBuffer::getPointerInternal() -> void *
	{
		PROFILE_FUNCTION();
		if (!mappedBuffer)
		{
			VulkanBuffer::map();
			mappedBuffer = true;
		}
		return mapped;
	}

	auto VulkanVertexBuffer::copy(CommandBuffer *cmd, GPUBuffer *to, const BufferCopy &copy) const -> void
	{
		auto buffer = static_cast<VulkanVertexBuffer *>(to);
		VkBufferCopy bufferCopy;
		bufferCopy.dstOffset = copy.dstOffset;
		bufferCopy.srcOffset = copy.srcOffset;
		bufferCopy.size      = copy.size;
		vkCmdCopyBuffer(static_cast<VulkanCommandBuffer *>(cmd)->getCommandBuffer(), getVkBuffer(), buffer->getVkBuffer(), 1, &bufferCopy);
	}
};        // namespace maple
