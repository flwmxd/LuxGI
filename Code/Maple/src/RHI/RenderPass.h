//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Definitions.h"
#include <glm/glm.hpp>

namespace maple
{
	class MAPLE_EXPORT RenderPass
	{
	  public:
		virtual ~RenderPass() = default;
		static auto create(const RenderPassInfo &renderPassDesc) -> std::shared_ptr<RenderPass>;

		virtual auto getAttachmentCount() const -> int32_t                                                                                                                                                                                         = 0;
		virtual auto beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, int32_t cubeFace = -1, int32_t mipMapLevel = 0) const -> void = 0;
		virtual auto beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, const glm::ivec4 &viewport) const -> void                     = 0;
		virtual auto endRenderPass(const CommandBuffer *commandBuffer) -> void                                                                                                                                                                     = 0;
	};
}        // namespace maple
