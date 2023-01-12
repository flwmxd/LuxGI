//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/IndexBuffer.h"
#include "VulkanBuffer.h"

#include <memory>

namespace maple
{
	class CommandBuffer;

	/**
	 * IndexBuffer for Vulkan 
	 */
	class VulkanIndexBuffer : public IndexBuffer, public VulkanBuffer
	{
	  public:
		VulkanIndexBuffer(const uint16_t *data, uint32_t count, BufferUsage bufferUsage, bool gpuOnly = false);
		VulkanIndexBuffer(const uint32_t *data, uint32_t count, BufferUsage bufferUsage, bool gpuOnly = false);
		~VulkanIndexBuffer();

		NO_COPYABLE(VulkanIndexBuffer);

		auto bind(const CommandBuffer *commandBuffer) const -> void override;
		auto unbind() const -> void override;
		auto setData(uint32_t size, const void *data) -> void;
		auto releasePointer() -> void override;

		auto copy(CommandBuffer *cmd, GPUBuffer *to, const BufferCopy &copy) const -> void override;

		auto getPointerInternal() -> void * override;

		inline auto getSize() const -> uint64_t override
		{
			return size;
		}

		inline auto setCount(uint32_t indexCount) -> void
		{
			count = indexCount;
		}
		inline auto getCount() const -> uint32_t override
		{
			return count;
		}
		inline auto getAddress() const -> uint64_t override
		{
			return getDeviceAddress();
		};
	  private:
		uint32_t    count = 0;
		uint32_t    size  = 0;
		BufferUsage usage;
		bool        mappedBuffer = false;
	};
};        // namespace maple
