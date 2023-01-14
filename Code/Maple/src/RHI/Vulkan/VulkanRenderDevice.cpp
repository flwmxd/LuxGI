//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanRenderDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanStorageBuffer.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"
#include "VulkanFramebuffer.h"

#include "RHI/Texture.h"

#include "Application.h"

namespace maple
{
	static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT = 3000;

	namespace
	{
		inline auto getStageFlags(ShaderType stage)
		{
			constexpr VkPipelineStageFlags types[14] =
			    {
			        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			        VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			        VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
			        VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
			        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			        VK_PIPELINE_STAGE_TRANSFER_BIT,
			        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};

			uint32_t retBits = 0;

			const ShaderType flags[13] = {
			    ShaderType::Vertex,
			    ShaderType::Fragment,
			    ShaderType::Geometry,
			    ShaderType::TessellationControl,
			    ShaderType::TessellationEvaluation,
			    ShaderType::Compute,
			    ShaderType::RayMiss,
			    ShaderType::RayCloseHit,
			    ShaderType::RayAnyHit,
			    ShaderType::RayGen,
			    ShaderType::RayIntersect,
			    ShaderType::TransferStage,
			    ShaderType::DrawIndirect};

			for (auto i = 0; i < 13; i++)
			{
				if (((uint32_t) stage & (uint32_t) flags[i]) == (uint32_t) flags[i])
				{
					retBits |= types[i];
				}
			}
			return retBits;
		}

		inline auto toAccessFlag(AccessFlags flags, VkPipelineStageFlags stageFlags) -> uint32_t
		{
			bool readOnly  = (flags & AccessFlags::Read) == AccessFlags::Read;
			bool writeOnly = (flags & AccessFlags::Write) == AccessFlags::Write;

			auto isTransfer     = (stageFlags & VK_PIPELINE_STAGE_TRANSFER_BIT) == VK_PIPELINE_STAGE_TRANSFER_BIT;
			auto isDrawIndirect = (stageFlags & VK_PIPELINE_STAGE_TRANSFER_BIT) == VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

			if (!isTransfer && !isDrawIndirect)
			{
				return readOnly ? VK_ACCESS_SHADER_READ_BIT : writeOnly ? VK_ACCESS_SHADER_WRITE_BIT :
                                                                          VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			}

			uint32_t flag = VK_ACCESS_NONE;
			if (isTransfer)
			{
				flag |= (readOnly ? VK_ACCESS_TRANSFER_READ_BIT : writeOnly ? VK_ACCESS_TRANSFER_WRITE_BIT :
                                                                              VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
			}

			if (isDrawIndirect)
			{
				MAPLE_ASSERT(writeOnly, "Draw-Indirect can not be write");
				flag |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			}

			return flag;
		}
	}        // namespace

	VulkanRenderDevice::VulkanRenderDevice()
	{
	}

	VulkanRenderDevice::~VulkanRenderDevice()
	{
		PROFILE_FUNCTION();
	}

	auto VulkanRenderDevice::init() -> void
	{
		PROFILE_FUNCTION();

		std::vector<DescriptorPoolInfo> poolInfos = {{DescriptorType::Image, 500},
		                                             {DescriptorType::ImageSampler, 500},
		                                             {DescriptorType::Buffer, 500},
		                                             {DescriptorType::BufferDynamic, 500},
		                                             {DescriptorType::UniformBuffer, 500},
		                                             {DescriptorType::UniformBufferDynamic, 500}};

		if (VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported())
		{
			poolInfos.push_back({DescriptorType::AccelerationStructure, 16});
		}

		descriptorPool = DescriptorPool::create({MAX_DESCRIPTOR_SET_COUNT, poolInfos});
	}

	auto VulkanRenderDevice::begin() -> void
	{
		PROFILE_FUNCTION();
		auto swapChain = Application::getGraphicsContext()->getSwapChain();
		std::static_pointer_cast<VulkanSwapChain>(swapChain)->begin();
	}

	auto VulkanRenderDevice::presentInternal() -> void
	{
		PROFILE_FUNCTION();
		auto swapChain = std::static_pointer_cast<VulkanSwapChain>(Application::getGraphicsContext()->getSwapChain());

		swapChain->end();
		swapChain->queueSubmit();

		swapChain->present();
	}

	auto VulkanRenderDevice::presentInternal(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
	}

	auto VulkanRenderDevice::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		if (width == 0 || height == 0)
			return;

		VulkanHelper::validateResolution(width, height);

		std::static_pointer_cast<VulkanSwapChain>(Application::getGraphicsContext()->getSwapChain())->onResize(width, height);
	}

	auto VulkanRenderDevice::drawInternal(const CommandBuffer *commandBuffer, const DrawType type, uint32_t count, DataType dataType, const void *indices) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		vkCmdDraw(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, 0, 0);
	}

	auto VulkanRenderDevice::drawArraysInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDraw(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, 0, 0);
	}

	auto VulkanRenderDevice::drawInstanced(const CommandBuffer *commandBuffer, uint32_t verticesCount, uint32_t instanceCount, int32_t startInstance, int32_t startVertex) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDraw(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), verticesCount, instanceCount, startVertex, startInstance);
	}

	auto VulkanRenderDevice::drawIndexedInternal(const CommandBuffer *commandBuffer, const DrawType type, uint32_t count, uint32_t start) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDrawIndexed(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, start, 0, 0);
	}

	auto VulkanRenderDevice::bindDescriptorSetsInternal(Pipeline *pipeline, const CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		PROFILE_FUNCTION();
		uint32_t numDynamicDescriptorSets = 0;
		uint32_t numDesciptorSets         = 0;

		for (auto &descriptorSet : descriptorSets)
		{
			if (descriptorSet)
			{
				auto vkDesSet = std::static_pointer_cast<VulkanDescriptorSet>(descriptorSet);
				if (vkDesSet->isDynamic())
					numDynamicDescriptorSets++;

				descriptorSetPool[numDesciptorSets] = vkDesSet->getDescriptorSet();
				numDesciptorSets++;
			}
		}

		vkCmdBindDescriptorSets(
		    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
		    static_cast<const VulkanPipeline *>(pipeline)->getPipelineBindPoint(),
		    static_cast<const VulkanPipeline *>(pipeline)->getPipelineLayout(), 0, numDesciptorSets, descriptorSetPool, numDynamicDescriptorSets, 0);
	}

	auto VulkanRenderDevice::bindDescriptorSets(Pipeline *pipeline, const CommandBuffer *commandBuffer, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		PROFILE_FUNCTION();
		uint32_t numDynamicDescriptorSets = 0;

		int32_t i = 0;
		for (auto &descriptorSet : descriptorSets)
		{
			if (descriptorSet)
			{
				auto vkDesSet = std::static_pointer_cast<VulkanDescriptorSet>(descriptorSet);
				if (vkDesSet->isDynamic())
				{
					numDynamicDescriptorSets++;
				}

				auto set = vkDesSet->getDescriptorSet();

				vkCmdBindDescriptorSets(
				    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
				    static_cast<const VulkanPipeline *>(pipeline)->getPipelineBindPoint(),
				    static_cast<const VulkanPipeline *>(pipeline)->getPipelineLayout(), i, 1, &set, 0, 0);
			}
			i++;
		}
	}

	auto VulkanRenderDevice::clearRenderTarget(const std::shared_ptr<Texture> &texture, const CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void
	{
		PROFILE_FUNCTION();
		if (texture)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.baseMipLevel            = 0;
			subresourceRange.layerCount              = 1;
			subresourceRange.levelCount              = 1;
			subresourceRange.baseArrayLayer          = 0;

			if (texture->getType() == TextureType::Color || texture->getType() == TextureType::Color3D)
			{
				auto vkTexture = (VulkanTexture2D *) texture.get();

				VkImageLayout layout = vkTexture->getImageLayout();
				vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VulkanCommandBuffer *) commandBuffer);
				subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
				VkClearColorValue clearColourValue = VkClearColorValue({{clearColor.x, clearColor.y, clearColor.z, clearColor.w}});
				vkCmdClearColorImage(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValue, 1, &subresourceRange);
				if (layout == VK_IMAGE_LAYOUT_UNDEFINED)
					layout = VK_IMAGE_LAYOUT_GENERAL;
				vkTexture->transitionImage(layout, (const VulkanCommandBuffer *) commandBuffer);
			}
			else if (texture->getType() == TextureType::Depth)
			{
				auto vkTexture = (VulkanTextureDepth *) texture.get();

				VkClearDepthStencilValue clearDepthStencil = {1.0f, 0};
				subresourceRange.aspectMask                = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VulkanCommandBuffer *) commandBuffer);
				vkCmdClearDepthStencilImage(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &subresourceRange);
			}
		}
	}

	auto VulkanRenderDevice::dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		PROFILE_FUNCTION();
		vkCmdDispatch(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), x, y, z);
	}

	auto VulkanRenderDevice::memoryBarrier(const CommandBuffer *commandBuffer, uint32_t flag) -> void
	{
		PROFILE_FUNCTION();
	}

	auto VulkanRenderDevice::memoryBarrier(const CommandBuffer *commandBuffer, ShaderType fromStage, ShaderType toStage, AccessFlags from, AccessFlags to) -> void
	{
		VkPipelineStageFlags srcStageMask = getStageFlags(fromStage);
		VkPipelineStageFlags dstStageMask = getStageFlags(toStage);

		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

		memoryBarrier.srcAccessMask = toAccessFlag(from, srcStageMask);
		memoryBarrier.dstAccessMask = toAccessFlag(to, dstStageMask);

		vkCmdPipelineBarrier(
		    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
		    srcStageMask,
		    dstStageMask,
		    0,
		    1,
		    &memoryBarrier,
		    0,
		    0,
		    0,
		    0);
	}

	auto VulkanRenderDevice::bufferMemoryBarrier(const CommandBuffer *commandBuffer, ShaderType fromStage, ShaderType toStage, const std::vector<BufferBarrier> &barries) const -> void
	{
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		VkPipelineStageFlags               srcStageMask = getStageFlags(fromStage);
		VkPipelineStageFlags               dstStageMask = getStageFlags(toStage);
		for (auto &barrier : barries)
		{
			auto                  ssbo          = static_cast<VulkanStorageBuffer *>(barrier.ssbo);
			VkBufferMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask         = toAccessFlag(barrier.from, srcStageMask);
			memoryBarrier.dstAccessMask         = toAccessFlag(barrier.to, dstStageMask);
			memoryBarrier.size                  = barrier.size;
			memoryBarrier.buffer                = ssbo->getHandle();
			memoryBarrier.srcQueueFamilyIndex   = *VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily;
			memoryBarrier.dstQueueFamilyIndex   = *VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily;
		}

		vkCmdPipelineBarrier(
		    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
		    srcStageMask, dstStageMask, 0, 0, 0, bufferBarriers.size(), bufferBarriers.data(), 0, 0);
	}

	auto VulkanRenderDevice::copyBuffer(const CommandBuffer *                 commandBuffer,
	                                    const std::shared_ptr<StorageBuffer> &from,
	                                    const std::shared_ptr<StorageBuffer> &to, uint32_t size, uint32_t dstOffset, uint32_t srcOffset, bool barrier) const -> void
	{
		auto vkFrom = std::static_pointer_cast<VulkanStorageBuffer>(from);
		auto vkTo   = std::static_pointer_cast<VulkanStorageBuffer>(to);

		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		if (barrier)
		{
			std::vector<VkBufferMemoryBarrier> bufferBarriers;

			VkBufferMemoryBarrier memoryBarrier{};
			uint32_t              srcAccessFlags = vkFrom->getAccessFlagBits();
			uint32_t              dstAccessFlags = srcAccessFlags | VK_ACCESS_TRANSFER_READ_BIT;
			memoryBarrier.sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask          = srcAccessFlags;
			memoryBarrier.dstAccessMask          = dstAccessFlags;
			memoryBarrier.buffer                 = vkFrom->getHandle();
			memoryBarrier.offset                 = 0;
			memoryBarrier.size                   = VK_WHOLE_SIZE;
			bufferBarriers.emplace_back(memoryBarrier);

			memoryBarrier.srcAccessMask = vkTo->getAccessFlagBits();
			memoryBarrier.dstAccessMask = vkTo->getAccessFlagBits() | VK_ACCESS_TRANSFER_WRITE_BIT;
			memoryBarrier.buffer        = vkTo->getHandle();
			bufferBarriers.emplace_back(memoryBarrier);

			vkCmdPipelineBarrier(
			    vkCmd->getCommandBuffer(),
			    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			    0,
			    0,
			    0,
			    bufferBarriers.size(),
			    bufferBarriers.data(),
			    0,
			    0);
		}

		VkBufferCopy bufferCopy;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.size      = size;
		vkCmdCopyBuffer(vkCmd->getCommandBuffer(), vkFrom->getHandle(), vkTo->getHandle(), 1, &bufferCopy);
	}

	auto VulkanRenderDevice::imageBarrier(const CommandBuffer *commandBuffer, const ImageMemoryBarrier &barriers) -> void
	{
		std::vector<VkImageMemoryBarrier> vkBarrier;
		VkPipelineStageFlags              srcStageMask = 0;
		VkPipelineStageFlags              dstStageMask = 0;

		for (auto &b : barriers.textures)
		{
			srcStageMask  = getStageFlags(barriers.fromStage);
			dstStageMask  = getStageFlags(barriers.toStage);
			auto &barrier = vkBarrier.emplace_back();
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			if (b->getType() == TextureType::Color)
			{
				VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
				bool                    readOnly         = (barriers.to & AccessFlags::Read) == AccessFlags::Read;

				auto vkTexture    = (VulkanTexture2D *) b.get();
				barrier.oldLayout = vkTexture->getImageLayout();
				barrier.newLayout = readOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
				vkTexture->setImageLayout(barrier.newLayout);
				barrier.subresourceRange = subresourceRange;
				barrier.image            = vkTexture->getImage();

				barrier.srcAccessMask = toAccessFlag(barriers.from, srcStageMask);
				barrier.dstAccessMask = toAccessFlag(barriers.to, dstStageMask);
				vkTexture->updateDescriptor();
			}
			else
			{
				MAPLE_ASSERT(false, "To be implementation !");
			}
		}

		if (!vkBarrier.empty())
		{
			vkCmdPipelineBarrier(
			    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
			    srcStageMask,
			    dstStageMask,
			    0,
			    0,
			    0,
			    0,
			    0,
			    vkBarrier.size(),
			    vkBarrier.data());
		}
	}
}        // namespace maple
