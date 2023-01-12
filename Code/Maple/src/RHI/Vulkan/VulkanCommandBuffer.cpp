//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanCommandBuffer.h"
#include "Others/Console.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"

#include "Application.h"

namespace maple
{
	VulkanCommandBuffer::VulkanCommandBuffer(CommandBufferType cmdBufferType) :
	    primary(false),
	    cmdBufferType(cmdBufferType)
	{
	}

	VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferType cmdBufferType) :
	    primary(true),
	    commandBuffer(commandBuffer),
	    cmdBufferType(cmdBufferType)
	{
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		unload();
	}

	auto VulkanCommandBuffer::init(bool primary) -> bool
	{
		fence         = std::make_shared<VulkanFence>(false);
		this->primary = primary;

		commandPool = *VulkanDevice::get()->getCommandPool();
		VkCommandBufferAllocateInfo cmdBufferCI{};
		cmdBufferCI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferCI.commandPool        = commandPool;
		cmdBufferCI.commandBufferCount = 1;
		cmdBufferCI.level              = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(*VulkanDevice::get(), &cmdBufferCI, &commandBuffer));

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext                 = nullptr;
		VK_CHECK_RESULT(vkCreateSemaphore(*VulkanDevice::get(), &semaphoreInfo, nullptr, &rendererSemaphore));

		return true;
	}

	auto VulkanCommandBuffer::init(bool primary, VkCommandPool cmdPool) -> bool
	{
		this->primary = primary;

		commandPool = cmdPool;
		VkCommandBufferAllocateInfo cmdBufferCI{};
		cmdBufferCI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferCI.commandPool        = commandPool;
		cmdBufferCI.commandBufferCount = 1;
		cmdBufferCI.level              = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(*VulkanDevice::get(), &cmdBufferCI, &commandBuffer));
		fence = std::make_shared<VulkanFence>(true);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext                 = nullptr;
		VK_CHECK_RESULT(vkCreateSemaphore(*VulkanDevice::get(), &semaphoreInfo, nullptr, &rendererSemaphore));

		return true;
	}

	/**
	 * destory the cmd
	 */
	auto VulkanCommandBuffer::unload() -> void
	{
		PROFILE_FUNCTION();
		Application::getGraphicsContext()->waitIdle();

		if (state == CommandBufferState::Submitted)
			wait();
		if (commandBuffer)
			vkFreeCommandBuffers(*VulkanDevice::get(), commandPool, 1, &commandBuffer);

		vkDestroySemaphore(*VulkanDevice::get(), rendererSemaphore, nullptr);
	}

	auto VulkanCommandBuffer::beginRecording() -> void
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(primary, "beginRecording() called from a secondary command buffer!");
		state = CommandBufferState::Recording;

		VkCommandBufferBeginInfo beginCI{};
		beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginCI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginCI));

		for (auto task : tasks)
		{
			task(this);
		}
		tasks.clear();
	}

	auto VulkanCommandBuffer::beginRecordingSecondary(RenderPass *renderPass, FrameBuffer *framebuffer) -> void
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(!primary, "beginRecordingSecondary() called from a primary command buffer!");
		state = CommandBufferState::Recording;

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.subpass     = 0;
		inheritanceInfo.renderPass  = *(VulkanRenderPass *) renderPass;
		inheritanceInfo.framebuffer = *(VulkanFrameBuffer *) framebuffer;

		VkCommandBufferBeginInfo beginCI{};
		beginCI.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginCI.flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginCI.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginCI));
	}

	auto VulkanCommandBuffer::endRecording() -> void
	{
		PROFILE_FUNCTION();

		MAPLE_ASSERT(state == CommandBufferState::Recording, "CommandBuffer ended before started recording");

		if (boundPipeline)
			boundPipeline->end(this);

		boundPipeline = nullptr;

#ifdef MAPLE_PROFILE
		//TracyVkCollect(VulkanDevice::get()->getTracyContext(), commandBuffer);
#endif        // MAPLE_PROFILE

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
		state = CommandBufferState::Ended;
	}

	auto VulkanCommandBuffer::executeSecondary(const CommandBuffer *primaryCmdBuffer) -> void
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(!primary, "Used ExecuteSecondary on primary command buffer!");
		state = CommandBufferState::Submitted;

		vkCmdExecuteCommands(static_cast<const VulkanCommandBuffer *>(primaryCmdBuffer)->getCommandBuffer(), 1, &commandBuffer);
	}

	auto VulkanCommandBuffer::updateViewport(uint32_t width, uint32_t height) const -> void
	{
		updateViewport(0, 0, width, height);
	}

	auto VulkanCommandBuffer::updateViewport(int32_t x, int32_t y, uint32_t width, uint32_t height) const -> void
	{
		PROFILE_FUNCTION();
		VkViewport viewport = {};
		viewport.x          = x >= 0 ? (float) x : 0;
		viewport.y          = y >= 0 ? (float) y : 0;
		viewport.width      = static_cast<float>(width);
		viewport.height     = static_cast<float>(height);
		viewport.minDepth   = 0.0f;
		viewport.maxDepth   = 1.0f;

		VkRect2D scissor = {};
		scissor.offset   = {x >= 0 ? x : 0, y >= 0 ? y : 0};
		scissor.extent   = {width, height};

		//viewport param
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		//scissor test.
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	auto VulkanCommandBuffer::bindPipeline(Pipeline *pipeline) -> void
	{
		if (pipeline != boundPipeline)
		{
			if (boundPipeline)
				boundPipeline->end(this);

			pipeline->bind(this);
			boundPipeline = pipeline;
		}
	}

	auto VulkanCommandBuffer::unbindPipeline() -> void
	{
		if (boundPipeline)
			boundPipeline->end(this);
		boundPipeline = nullptr;
	}

	auto VulkanCommandBuffer::flush() -> bool
	{
		if (state != CommandBufferState::Idle)
		{
			Application::getGraphicsContext()->waitIdle();
			if (state == CommandBufferState::Submitted)
				wait();
		}
		return true;
	}

	auto VulkanCommandBuffer::endSingleTimeCommands() -> void
	{
		VulkanHelper::endSingleTimeCommands(commandBuffer);
	}

	auto VulkanCommandBuffer::executeInternal(const std::vector<VkPipelineStageFlags> &flags,
	                                          const std::vector<VkSemaphore> &         waitSemaphores,
	                                          const std::vector<VkSemaphore> &         signalSemaphores,
	                                          bool                                     waitFence) -> void
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(primary, "Used Execute on secondary command buffer!");
		MAPLE_ASSERT(state == CommandBufferState::Ended, "CommandBuffer executed before ended recording");

		VkSubmitInfo submitInfo       = {};
		submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext              = VK_NULL_HANDLE;
		submitInfo.waitSemaphoreCount = waitSemaphores.size();
		submitInfo.pWaitSemaphores    = waitSemaphores.data();
		submitInfo.pWaitDstStageMask  = flags.data();

		submitInfo.commandBufferCount   = 1;
		submitInfo.pCommandBuffers      = &commandBuffer;
		submitInfo.signalSemaphoreCount = signalSemaphores.size();
		submitInfo.pSignalSemaphores    = signalSemaphores.data();

		fence->reset();

		VK_CHECK_RESULT(vkQueueSubmit(
		    cmdBufferType == CommandBufferType::Graphics ?
                VulkanDevice::get()->getGraphicsQueue() :
                VulkanDevice::get()->getComputeQueue(),
		    1, &submitInfo, fence->getHandle()));
		state = CommandBufferState::Submitted;
	}

	auto VulkanCommandBuffer::addTask(const std::function<void(const CommandBuffer *)> &task) -> void
	{
		tasks.emplace_back(task);
	}

	auto VulkanCommandBuffer::wait() -> void
	{
		PROFILE_FUNCTION();
		MAPLE_ASSERT(state == CommandBufferState::Submitted, "");
		fence->waitAndReset();
		state = CommandBufferState::Idle;
	}

	auto VulkanCommandBuffer::reset() -> void
	{
		PROFILE_FUNCTION();
		VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffer, 0));
	}
};        // namespace maple
