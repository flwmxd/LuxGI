//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanStorageBuffer.h"
#include "VulkanBuffer.h"

namespace maple
{
	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, const void *data, const BufferOptions &options) :
	    options(options)
	{
		accessFlagBits = VK_ACCESS_SHADER_READ_BIT;

		VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		if (options.indirect)
		{
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			accessFlagBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		}

		lastAccessFlagBits = accessFlagBits;
		vulkanBuffer       = std::make_shared<VulkanBuffer>(flags, size, data, options.vmaUsage, options.vmaCreateFlags);
	}

	VulkanStorageBuffer::VulkanStorageBuffer(const BufferOptions &options) :
	    options(options)
	{
		vulkanBuffer = std::make_shared<VulkanBuffer>();
	}

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, uint32_t flags, const BufferOptions &options)
	{
		vulkanBuffer = std::make_shared<VulkanBuffer>(flags, size, nullptr, options.vmaUsage, options.vmaCreateFlags);
	}

	auto VulkanStorageBuffer::setData(uint32_t size, const void *data) -> void
	{
		if (vulkanBuffer->getSize() == 0)
		{
			VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			accessFlagBits           = VK_ACCESS_SHADER_READ_BIT;

			if (options.indirect)
			{
				flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
				accessFlagBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			}

			lastAccessFlagBits = accessFlagBits;
			vulkanBuffer->init(flags, size, data, options.vmaUsage, options.vmaCreateFlags);
		}
		else
		{
			vulkanBuffer->setVkData(size, data);
		}
	}

	auto VulkanStorageBuffer::getHandle() const -> VkBuffer &
	{
		return vulkanBuffer->getVkBuffer();
	}

	auto VulkanStorageBuffer::mapMemory(const std::function<void(void *)> &call) -> void
	{
		vulkanBuffer->map();
		call(vulkanBuffer->getMapped());
		vulkanBuffer->unmap();
	}

	auto VulkanStorageBuffer::unmap() -> void
	{
		vulkanBuffer->unmap();
	}

	auto VulkanStorageBuffer::map() -> void *
	{
		vulkanBuffer->map();
		return vulkanBuffer->getMapped();
	}

	auto VulkanStorageBuffer::getDeviceAddress() const -> uint64_t
	{
		return vulkanBuffer->getDeviceAddress();
	}

	auto VulkanStorageBuffer::getSize() const -> uint32_t
	{
		return vulkanBuffer->getSize();
	}

	auto VulkanStorageBuffer::resize(uint32_t size) -> void
	{
		dirty = true;
		vulkanBuffer->release();

		VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		accessFlagBits           = VK_ACCESS_SHADER_READ_BIT;

		if (options.indirect)
		{
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			accessFlagBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		}

		lastAccessFlagBits = accessFlagBits;
		vulkanBuffer->init(flags, size, nullptr, options.vmaUsage, options.vmaCreateFlags);
	}

	auto VulkanStorageBuffer::setAccessFlagBits(uint32_t flags) -> void
	{
		lastAccessFlagBits = accessFlagBits;
		accessFlagBits     = flags;
	}

	auto VulkanStorageBuffer::isDirty() const -> bool 
	{
		return dirty;
	}

	auto VulkanStorageBuffer::setDirty(bool dirty) -> void 
	{
		this->dirty = dirty;
	}
}        // namespace maple