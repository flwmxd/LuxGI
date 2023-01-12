
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanRenderPass.h"
#include "Others/Console.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanHelper.h"
#include "VulkanTexture.h"

namespace maple
{
	inline auto subPassContentsToVK(SubPassContents contents) -> VkSubpassContents
	{
		switch (contents)
		{
			case SubPassContents::Inline:
				return VK_SUBPASS_CONTENTS_INLINE;
			case SubPassContents ::Secondary:
				return VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
			default:
				return VK_SUBPASS_CONTENTS_INLINE;
		}
	}

	inline auto getAttachmentDescription(std::shared_ptr<Texture> texture, bool clear = true) -> VkAttachmentDescription
	{
		VkAttachmentDescription attachment = {};
		if (texture->getType() == TextureType::Color)
		{
			auto colorTexture = std::static_pointer_cast<VulkanTexture2D>(texture);
			attachment.format = colorTexture->getVkFormat();

			MAPLE_ASSERT(attachment.format != VK_FORMAT_UNDEFINED, "");

			attachment.initialLayout = colorTexture->getImageLayout();
			attachment.finalLayout   = attachment.initialLayout;
		}
		else if (texture->getType() == TextureType::Depth)
		{
			attachment.format        = std::static_pointer_cast<VulkanTextureDepth>(texture)->getVkFormat();
			attachment.initialLayout = std::static_pointer_cast<VulkanTextureDepth>(texture)->getImageLayout();
			attachment.finalLayout   = attachment.initialLayout;
			MAPLE_ASSERT(attachment.format != VK_FORMAT_UNDEFINED, "");
		}
		else if (texture->getType() == TextureType::DepthArray)
		{
			attachment.format        = VulkanHelper::getDepthFormat();
			attachment.initialLayout = std::static_pointer_cast<VulkanTextureDepthArray>(texture)->getImageLayout();
			attachment.finalLayout   = attachment.initialLayout;
			MAPLE_ASSERT(attachment.format != VK_FORMAT_UNDEFINED, "");
		}
		else if (texture->getType() == TextureType::Cube)
		{
			/*	attachment.format        = std::static_pointer_cast<VulkanTextureCube>(texture)->getVkFormat();
			attachment.initialLayout = std::static_pointer_cast<VulkanTextureCube>(texture)->getImageLayout();
			attachment.finalLayout   = attachment.initialLayout;
			MAPLE_ASSERT(attachment.format != VK_FORMAT_UNDEFINED, "");*/
		}
		else
		{
			LOGC("[VULKAN] - Unsupported TextureType - {0}", int32_t(texture->getType()));
			return attachment;
		}

		if (clear)
		{
			attachment.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else
		{
			attachment.loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		}

		attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.flags          = 0;

		return attachment;
	}

	VulkanRenderPass::VulkanRenderPass(const RenderPassInfo &info)
	{
		init(info);
	}

	auto VulkanRenderPass::init(const RenderPassInfo &info) -> void
	{
		PROFILE_FUNCTION();
		std::vector<VkAttachmentDescription> attachments;

		std::vector<VkAttachmentReference> colorAttachmentReferences;
		std::vector<VkAttachmentReference> depthAttachmentReferences;

		depthOnly  = true;
		clearDepth = false;

		for (uint32_t i = 0; i < info.attachments.size(); i++)
		{
			auto texture = info.attachments[i];

			if (texture->getType() == TextureType::Cube)
			{
				LOGW("CubeMap could not be used as an attachment");
				continue;
			}

			attachments.emplace_back(getAttachmentDescription(info.attachments[i], info.clear));

			if (texture->getType() == TextureType::Color)
			{
				VkImageLayout         layout             = std::static_pointer_cast<VulkanTexture2D>(texture)->getImageLayout();
				VkAttachmentReference colorAttachmentRef = {};
				colorAttachmentRef.attachment            = uint32_t(i);
				colorAttachmentRef.layout                = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
				colorAttachmentReferences.push_back(colorAttachmentRef);
				depthOnly = false;
			}
			else if (texture->getType() == TextureType::Depth)
			{
				VkAttachmentReference depthAttachmentRef = {};
				depthAttachmentRef.attachment            = uint32_t(i);
				depthAttachmentRef.layout                = std::static_pointer_cast<VulkanTextureDepth>(texture)->getImageLayout();
				depthAttachmentReferences.push_back(depthAttachmentRef);
				clearDepth = info.clear;
			}
			else if (texture->getType() == TextureType::DepthArray)
			{
				VkAttachmentReference depthAttachmentRef = {};
				depthAttachmentRef.attachment            = uint32_t(i);
				depthAttachmentRef.layout                = std::static_pointer_cast<VulkanTextureDepthArray>(texture)->getImageLayout();
				depthAttachmentReferences.push_back(depthAttachmentRef);
				clearDepth = info.clear;
			}
			else if (texture->getType() == TextureType::Cube)
			{
				/*VkAttachmentReference colorAttachmentRef = {};
				colorAttachmentRef.attachment            = uint32_t(i);
				colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentReferences.emplace_back(colorAttachmentRef);
				depthOnly = false;*/
			}
			else
			{
				LOGE("Unsupported texture attachment");
			}
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentReferences.size());
		subpass.pColorAttachments       = colorAttachmentReferences.data();
		subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

		colorAttachmentCount = int32_t(colorAttachmentReferences.size());

		VkRenderPassCreateInfo renderPassCreateInfo = VulkanHelper::renderPassCreateInfo();
		renderPassCreateInfo.attachmentCount        = uint32_t(attachments.size());
		renderPassCreateInfo.pAttachments           = attachments.data();
		renderPassCreateInfo.subpassCount           = 1;
		renderPassCreateInfo.pSubpasses             = &subpass;
		renderPassCreateInfo.dependencyCount        = 0;
		renderPassCreateInfo.pDependencies          = nullptr;

		VK_CHECK_RESULT(vkCreateRenderPass(*VulkanDevice::get(), &renderPassCreateInfo, VK_NULL_HANDLE, &renderPass));

		clearValue = new VkClearValue[attachments.size()];
		memset(clearValue, 0, sizeof(VkClearValue) * attachments.size());
		clearCount = attachments.size();
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		auto renderPass = this->renderPass;
		VulkanContext::getDeletionQueue().emplace([renderPass]() {
			vkDestroyRenderPass(*VulkanDevice::get(), renderPass, nullptr);
		});
		delete[] clearValue;
	}

	auto VulkanRenderPass::beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, int32_t cubeFace, int32_t mipMapLevel) const -> void
	{
		PROFILE_FUNCTION();
		if (!depthOnly)
		{
			for (int32_t i = 0; i < clearCount; i++)
			{
				clearValue[i].color.float32[0] = clearColor.x;
				clearValue[i].color.float32[1] = clearColor.y;
				clearValue[i].color.float32[2] = clearColor.z;
				clearValue[i].color.float32[3] = clearColor.w;
			}
		}

		if (clearDepth)
		{
			clearValue[clearCount - 1].depthStencil = VkClearDepthStencilValue{1.0f, 0};
		}

		VkRenderPassBeginInfo info{};
		info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext                    = nullptr;
		info.renderPass               = renderPass;
		info.framebuffer              = *static_cast<VulkanFrameBuffer *>(frame);
		info.renderArea.offset.x      = 0;
		info.renderArea.offset.y      = 0;
		info.renderArea.extent.width  = width;
		info.renderArea.extent.height = height;
		info.clearValueCount          = uint32_t(clearCount);
		info.pClearValues             = clearValue;

		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		MAPLE_ASSERT(vkCmd->isRecording(), "must recording");

		vkCmdBeginRenderPass(vkCmd->getCommandBuffer(), &info, subPassContentsToVK(contents));
		commandBuffer->updateViewport(width, height);
	}

	auto VulkanRenderPass::beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame,
		SubPassContents contents, uint32_t width, uint32_t height, const glm::ivec4 &viewport) const -> void
	{
		PROFILE_FUNCTION();
		if (!depthOnly)
		{
			for (int32_t i = 0; i < clearCount; i++)
			{
				clearValue[i].color.float32[0] = clearColor.x;
				clearValue[i].color.float32[1] = clearColor.y;
				clearValue[i].color.float32[2] = clearColor.z;
				clearValue[i].color.float32[3] = clearColor.w;
			}
		}

		if (clearDepth)
		{
			clearValue[clearCount - 1].depthStencil = VkClearDepthStencilValue{1.0f, 0};
		}

		VkRenderPassBeginInfo info{};
		info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.pNext                    = nullptr;
		info.renderPass               = renderPass;
		info.framebuffer              = *static_cast<VulkanFrameBuffer *>(frame);
		info.renderArea.offset.x      = 0;
		info.renderArea.offset.y      = 0;
		info.renderArea.extent.width  = width;
		info.renderArea.extent.height = height;
		info.clearValueCount          = uint32_t(clearCount);
		info.pClearValues             = clearValue;

		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		MAPLE_ASSERT(vkCmd->isRecording(), "must recording");

		vkCmdBeginRenderPass(vkCmd->getCommandBuffer(), &info, subPassContentsToVK(contents));
		if (viewport.x >= 0)
			commandBuffer->updateViewport(viewport.x, viewport.y, viewport.z, viewport.w);
		else
			commandBuffer->updateViewport(width, height);
	}

	auto VulkanRenderPass::endRenderPass(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		vkCmdEndRenderPass(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer());
	}
};        // namespace maple
