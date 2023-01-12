//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RHI/UniformBuffer.h"
#include "VulkanBuffer.h"

namespace maple
{
	class VulkanUniformBuffer : public VulkanBuffer, public UniformBuffer
	{
	  public:
		VulkanUniformBuffer(uint32_t size, const void *data);
		VulkanUniformBuffer();
		~VulkanUniformBuffer();

		auto init(uint32_t size, const void *data) -> void override;
		auto setDynamicData(uint32_t size, uint32_t typeSize, const void *data) -> void override;
		auto setData(uint32_t size, const void *data) -> void override;

		inline auto setData(const void *data) -> void override
		{
			setData((uint32_t) size, data);
		};

		inline auto getBuffer() const -> uint8_t * override
		{
			return nullptr;
		}

		inline auto getMemory()
		{
			return &memory;
		}
	};
};        // namespace maple
