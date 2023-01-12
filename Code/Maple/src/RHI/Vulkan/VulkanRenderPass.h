//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/RenderPass.h"
#include "VulkanHelper.h"

namespace maple
{
	class VulkanRenderPass : public RenderPass
	{
	  public:
		VulkanRenderPass(const RenderPassInfo &info);
		virtual ~VulkanRenderPass();
		NO_COPYABLE(VulkanRenderPass);
		auto init(const RenderPassInfo &info) -> void;

		auto beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, int32_t cubeFace, int32_t mipMapLevel) const -> void override;
		auto beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, const glm::ivec4 &viewport) const -> void override;

		auto endRenderPass(const CommandBuffer *commandBuffer) -> void override;

		inline auto getAttachmentCount() const -> int32_t override
		{
			return clearCount;
		}

		inline auto getColorAttachmentCount() const
		{
			return colorAttachmentCount;
		}

		autoUnpack(renderPass);

	  private:
		VkClearValue *clearValue = nullptr;
		int32_t       clearCount = 0;
		int32_t       colorAttachmentCount;
		bool          clearDepth = false;
		bool          depthOnly  = true;
		VkRenderPass  renderPass = nullptr;
	};
};        // namespace maple
