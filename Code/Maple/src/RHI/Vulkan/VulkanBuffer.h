//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Vk.h"

#include "Engine/Core.h"
namespace maple
{
	class VulkanBuffer
	{
	  public:
		using Ptr = std::shared_ptr<VulkanBuffer>;

		VulkanBuffer();
#ifdef USE_VMA_ALLOCATOR
		VulkanBuffer(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU, uint32_t vmaCreateFlags = 0);
#else
		VulkanBuffer(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage = 3, uint32_t vmaCreateFlags = 0);
#endif        // USE_VMA_ALLOCATOR
		virtual ~VulkanBuffer();
		NO_COPYABLE(VulkanBuffer);

		auto resize(uint32_t size, const void *data) -> void;
#ifdef USE_VMA_ALLOCATOR	
		auto init(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU, uint32_t vmaCreateFlags = 0) -> void;
#else
		auto init(VkBufferUsageFlags usage, uint32_t size, const void *data, uint32_t vmaUsage = 3, uint32_t vmaCreateFlags = 0) -> void;
#endif
		auto map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto unmap() -> void;
		auto flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto setVkData(uint32_t size, const void *data, uint32_t offset = 0) -> void;

		inline auto setUsage(VkBufferUsageFlags flags)
		{
			usage = flags;
		}
		inline auto &getVkBuffer() 
		{
			return buffer;
		}
		inline const auto &getVkBuffer() const
		{
			return buffer;
		}
		inline const auto &getBufferInfo() const
		{
			return desciptorBufferInfo;
		}

		auto getDeviceAddress() const -> VkDeviceAddress;

		inline auto getSize() const
		{
			return size;
		}

		inline auto getMapped()
		{
			return mapped;
		}

		auto release() -> void;

	  protected:
		VkDeviceAddress        address = 0;
		VkDescriptorBufferInfo desciptorBufferInfo{};
		VkBuffer               buffer    = VK_NULL_HANDLE;
		VkDeviceMemory         memory    = VK_NULL_HANDLE;
		VkDeviceSize           size      = 0;
		VkDeviceSize           alignment = 0;
		void *                 mapped    = nullptr;
		VkBufferUsageFlags     usage;
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
		VmaAllocation mappedAllocation{};
#endif
		bool dynamic = false;
	};
}        // namespace maple
