//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "BufferUsage.h"
#include "Engine/Core.h"
#include "GPUBuffer.h"
#include <memory>

namespace maple
{
	class CommandBuffer;
	class Pipeline;

	class MAPLE_EXPORT VertexBuffer : public GPUBuffer
	{
	  public:
		using Ptr                                                                         = std::shared_ptr<VertexBuffer>;
		virtual ~VertexBuffer()                                                           = default;
		virtual auto resize(uint32_t size) -> void                                        = 0;
		virtual auto setData(uint32_t size, const void *data) -> void                     = 0;
		virtual auto setDataSub(uint32_t size, const void *data, uint32_t offset) -> void = 0;
		virtual auto releasePointer() -> void                                             = 0;
		virtual auto bind(const CommandBuffer *commandBuffer, Pipeline *pipeline) -> void = 0;
		virtual auto unbind() -> void                                                     = 0;
		virtual auto getSize() -> uint64_t
		{
			return 0;
		}

		template <typename T>
		inline auto getPointer() -> T *
		{
			return static_cast<T *>(getPointerInternal());
		}

		virtual auto getAddress() const -> uint64_t
		{
			return 0;
		};

	  protected:
		virtual void *getPointerInternal() = 0;

	  public:
		static auto create(const BufferUsage &usage = BufferUsage::Static) -> std::shared_ptr<VertexBuffer>;
		static auto create(const void *data, uint32_t size, bool gpuOnly = false) -> std::shared_ptr<VertexBuffer>;
	};
}        // namespace maple
