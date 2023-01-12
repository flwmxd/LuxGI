//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace maple
{
	class UniformBuffer
	{
	  public:
		virtual ~UniformBuffer() = default;
		static auto create() -> std::shared_ptr<UniformBuffer>;
		static auto create(uint32_t size, const void *data) -> std::shared_ptr<UniformBuffer>;

		virtual auto init(uint32_t size, const void *data) -> void                              = 0;
		virtual auto setData(const void *data) -> void                                          = 0;
		virtual auto setData(uint32_t size, const void *data) -> void                           = 0;
		virtual auto setDynamicData(uint32_t size, uint32_t typeSize, const void *data) -> void = 0;
		virtual auto getBuffer() const -> uint8_t *                                             = 0;
	};
}        // namespace maple
