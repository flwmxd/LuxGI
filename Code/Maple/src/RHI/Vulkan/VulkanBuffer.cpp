//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanBuffer.h"
#include "Others/Console.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanHelper.h"
#include "VulkanSwapChain.h"

namespace maple
{
	VulkanBuffer::VulkanBuffer()
	{
	}

	VulkanBuffer::VulkanBuffer(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage, uint32_t vmaCreateFlags) :
	    usage(usage),
	    size(size)
	{
		init(usage, size, data, vmaUsage, vmaCreateFlags);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		release();
	}

	auto VulkanBuffer::init(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage, uint32_t vmaCreateFlags) -> void
	{
		PROFILE_FUNCTION();
		//param for creating
		this->usage                   = usage;
		this->size                    = size;
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size               = size;
		bufferInfo.usage              = usage;
		bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

#ifdef USE_VMA_ALLOCATOR

		VkMemoryPropertyFlags memoryPropFlags = 0;

		if (vmaUsage == VMA_MEMORY_USAGE_CPU_ONLY)
		{
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		else if (vmaUsage == VMA_MEMORY_USAGE_GPU_ONLY)
		{
			memoryPropFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		else if (vmaUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)
		{
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		else if (vmaUsage == VMA_MEMORY_USAGE_GPU_TO_CPU)
		{
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		if (dynamic)//CPU-TO-GPU
		{
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			memoryPropFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaCreateInfo = {};
		vmaCreateInfo.flags                   = vmaCreateFlags;
		vmaCreateInfo.usage                   = (VmaMemoryUsage) vmaUsage;
		vmaCreateInfo.requiredFlags           = memoryPropFlags;

		vmaCreateBuffer(VulkanDevice::get()->getAllocator(), &bufferInfo, &vmaCreateInfo, &buffer, &allocation, nullptr);

#else
		VK_CHECK_RESULT(vkCreateBuffer(*VulkanDevice::get(), &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(*VulkanDevice::get(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize       = memRequirements.size;
		allocInfo.memoryTypeIndex      = VulkanHelper::findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK_RESULT(vkAllocateMemory(*VulkanDevice::get(), &allocInfo, nullptr, &memory));
		//bind buffer ->
		vkBindBufferMemory(*VulkanDevice::get(), buffer, memory, 0);
		//if the data is not nullptr, upload the data.
#endif
		if (data != nullptr)
			setVkData(size, data);

		VkBufferDeviceAddressInfoKHR addressInfo{};

		addressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.buffer = buffer;

		if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
			address = vkGetBufferDeviceAddress(*VulkanDevice::get(), &addressInfo);
	}

	/**
	 * map the data
	 */
	auto VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) -> void
	{
		PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
		VK_CHECK_RESULT(vmaMapMemory(VulkanDevice::get()->getAllocator(), allocation, &mapped));
#else
		VK_CHECK_RESULT(vkMapMemory(*VulkanDevice::get(), memory, offset, size, 0, &mapped));
#endif        // USE_
	}
	/**
	 * unmap the data.
	 */
	auto VulkanBuffer::unmap() -> void
	{
		PROFILE_FUNCTION();
		if (mapped)
		{
#ifdef USE_VMA_ALLOCATOR
			vmaUnmapMemory(VulkanDevice::get()->getAllocator(), allocation);
#else
			vkUnmapMemory(*VulkanDevice::get(), memory);
#endif
			mapped = nullptr;
		}
	}

	auto VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) -> void
	{
		PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
		vmaFlushAllocation(VulkanDevice::get()->getAllocator(), allocation, offset, size);
#else
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory              = memory;
		mappedRange.offset              = offset;
		mappedRange.size                = size;
		VK_CHECK_RESULT(vkFlushMappedMemoryRanges(*VulkanDevice::get(), 1, &mappedRange));
#endif
	}

	auto VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) -> void
	{
		PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
		vmaInvalidateAllocation(VulkanDevice::get()->getAllocator(), allocation, offset, size);
#else
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory              = memory;
		mappedRange.offset              = offset;
		mappedRange.size                = size;
		VK_CHECK_RESULT(vkInvalidateMappedMemoryRanges(*VulkanDevice::get(), 1, &mappedRange));
#endif
	}

	auto VulkanBuffer::setVkData(uint32_t size, const void *data, uint32_t offset) -> void
	{
		PROFILE_FUNCTION();
		if (data != nullptr)
		{
			map(size, offset);
			memcpy(reinterpret_cast<uint8_t *>(mapped) + offset, data, size);
			unmap();
		}
	}

	auto VulkanBuffer::getDeviceAddress() const -> VkDeviceAddress
	{
		MAPLE_ASSERT(address != 0, "address should be not zero");
		return address;
	}

	auto VulkanBuffer::resize(uint32_t size, const void *data) -> void
	{
		PROFILE_FUNCTION();
		release();
		init(usage, size, data);
	}

	auto VulkanBuffer::release() -> void
	{
		PROFILE_FUNCTION();
		if (buffer)
		{
			auto &queue    = VulkanContext::getDeletionQueue();
			auto  buffer   = this->buffer;
			auto  bufferId = VulkanContext::get()->getSwapChain()->getCurrentBufferIndex();
#ifdef USE_VMA_ALLOCATOR
			auto alloc = allocation;
			unmap();
			queue.emplace([buffer, alloc, bufferId] { vmaDestroyBuffer(VulkanDevice::get()->getAllocator(), buffer, alloc); });
#else

			auto memory = this->memory;
			queue.emplace([buffer, memory]() {
				vkDestroyBuffer(*VulkanDevice::get(), buffer, nullptr);
				if (memory)
				{
					vkFreeMemory(*VulkanDevice::get(), memory, nullptr);
				}
			});
#endif
		}
	}
};        // namespace maple
