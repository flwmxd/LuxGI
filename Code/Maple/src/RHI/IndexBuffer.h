//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "BufferUsage.h"
#include "Engine/Core.h"
#include "GPUBuffer.h"

namespace maple
{
	class CommandBuffer;

	class MAPLE_EXPORT IndexBuffer : public GPUBuffer
	{
	  public:
		using Ptr = std::shared_ptr<IndexBuffer>;

		virtual ~IndexBuffer()                                                        = default;
		virtual auto bind(const CommandBuffer *commandBuffer = nullptr) const -> void = 0;
		virtual auto unbind() const -> void                                           = 0;
		virtual auto getCount() const -> uint32_t                                     = 0;
		virtual auto setCount(uint32_t indexCount) -> void                            = 0;

		virtual auto releasePointer() -> void{};

		virtual auto getSize() const -> uint64_t
		{
			return 0;
		}

		template <typename T>
		inline auto getPointer()
		{
			return static_cast<T *>(getPointerInternal());
		}

		virtual auto getAddress() const -> uint64_t
		{
			return 0;
		};

	  public:
		static auto create(const uint16_t *data, uint32_t count, BufferUsage bufferUsage = BufferUsage::Static, bool gpuOnly = false) -> std::shared_ptr<IndexBuffer>;
		static auto create(const uint32_t *data, uint32_t count, BufferUsage bufferUsage = BufferUsage::Static, bool gpuOnly = false) -> std::shared_ptr<IndexBuffer>;

	  protected:
		virtual auto getPointerInternal() -> void *
		{
			return nullptr;
		}
	};
}        // namespace maple
