//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine									//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanSwapChain.h"
#include "Others/Console.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanHelper.h"
#include "VulkanTexture.h"

#include "Application.h"

#include "Window/NativeWindow.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace maple
{
	namespace
	{
		inline auto createPlatformSurface(NativeWindow *window)
		{
			VkSurfaceKHR surface;
			glfwCreateWindowSurface(VulkanContext::get()->getVkInstance(), static_cast<GLFWwindow *>(window->getNativeInterface()), nullptr, (VkSurfaceKHR *) &surface);
			return surface;
		}
	}        // namespace

	VulkanSwapChain::VulkanSwapChain(uint32_t width, uint32_t height) :
	    width(width),
	    height(height)
	{
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		for (uint32_t i = 0; i < swapChainBufferCount; i++)
		{
			frames[i].commandBuffer->flush();
			vkDestroySemaphore(*VulkanDevice::get(), frames[i].presentSemaphore, nullptr);
			frames[i].commandBuffer = nullptr;
		}
		vkDestroySwapchainKHR(*VulkanDevice::get(), swapChain, VK_NULL_HANDLE);

		if (surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(VulkanContext::get()->getVkInstance(), surface, nullptr);
		}

		vkDestroySwapchainKHR(*VulkanDevice::get(), swapChain, VK_NULL_HANDLE);
	}

	auto VulkanSwapChain::init(bool vsync, NativeWindow *window) -> bool
	{
		PROFILE_FUNCTION();
		this->vsync = vsync;

		if (surface == VK_NULL_HANDLE)
			surface = createPlatformSurface(window);

		bool success = init(vsync);
		acquireNextImage();
		return success;
	}

	auto VulkanSwapChain::init(bool vsync) -> bool
	{
		PROFILE_FUNCTION();
		findImageFormatAndColorSpace();

		if (!surface)
		{
			LOGC("[VULKAN] Failed to create window surface!");
		}

		VkBool32 queueIndexSupported;
		vkGetPhysicalDeviceSurfaceSupportKHR(*VulkanDevice::get()->getPhysicalDevice(),
		                                     VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily.value(),
		                                     surface, &queueIndexSupported);

		if (queueIndexSupported == VK_FALSE)
			LOGE("Graphics Queue not supported");

		// Swap chain
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*VulkanDevice::get()->getPhysicalDevice(), surface, &surfaceCapabilities);

		uint32_t numPresentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(*VulkanDevice::get()->getPhysicalDevice(), surface, &numPresentModes, VK_NULL_HANDLE);

		std::vector<VkPresentModeKHR> pPresentModes(numPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(*VulkanDevice::get()->getPhysicalDevice(), surface, &numPresentModes, pPresentModes.data());

		VkExtent2D swapChainExtent;

		swapChainExtent.width  = static_cast<uint32_t>(width);
		swapChainExtent.height = static_cast<uint32_t>(height);

		auto swapChainPresentMode = VulkanHelper::chooseSwapPresentMode(pPresentModes, vsync);

		//triple-buffering
		swapChainBufferCount = surfaceCapabilities.maxImageCount;

		if (swapChainBufferCount > 2)
			swapChainBufferCount = 2;

		VkSurfaceTransformFlagBitsKHR preTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			preTransform = surfaceCapabilities.currentTransform;
		}

		VkCompositeAlphaFlagBitsKHR              compositeAlpha      = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};

		for (auto &compositeAlphaFlag : compositeAlphaFlags)
		{
			if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
			{
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapChainCI{};
		swapChainCI.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCI.surface               = surface;
		swapChainCI.minImageCount         = swapChainBufferCount;
		swapChainCI.imageFormat           = colorFormat;
		swapChainCI.imageExtent.width     = swapChainExtent.width;
		swapChainCI.imageExtent.height    = swapChainExtent.height;
		swapChainCI.preTransform          = preTransform;
		swapChainCI.compositeAlpha        = compositeAlpha;
		swapChainCI.imageArrayLayers      = 1;
		swapChainCI.presentMode           = swapChainPresentMode;
		swapChainCI.oldSwapchain          = oldSwapChain;
		swapChainCI.imageColorSpace       = colorSpace;
		swapChainCI.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCI.queueFamilyIndexCount = 0;
		swapChainCI.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCI.pQueueFamilyIndices   = VK_NULL_HANDLE;
		swapChainCI.clipped               = VK_TRUE;

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
		{
			swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VK_CHECK_RESULT(vkCreateSwapchainKHR(*VulkanDevice::get(), &swapChainCI, VK_NULL_HANDLE, &swapChain));

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(*VulkanDevice::get(), oldSwapChain, VK_NULL_HANDLE);
			oldSwapChain = VK_NULL_HANDLE;

			for (uint32_t i = 0; i < swapChainBufferCount; i++)
			{
				if (frames[i].commandBuffer->getState() == CommandBufferState::Submitted)
					frames[i].commandBuffer->wait();

				frames[i].commandBuffer->reset();

				vkDestroySemaphore(*VulkanDevice::get(), frames[i].presentSemaphore, nullptr);
				frames[i].presentSemaphore = VK_NULL_HANDLE;
			}
		}

		uint32_t swapChainImageCount;
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(*VulkanDevice::get(), swapChain, &swapChainImageCount, VK_NULL_HANDLE));

		VkImage *pSwapChainImages = new VkImage[swapChainImageCount];
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(*VulkanDevice::get(), swapChain, &swapChainImageCount, pSwapChainImages));

		for (uint32_t i = 0; i < swapChainBufferCount; i++)
		{
			VkImageViewCreateInfo viewCI{};
			viewCI.sType  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCI.format = colorFormat;
#ifdef PLATFORM_MACOS
			viewCI.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
#else
			viewCI.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
#endif
			viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCI.subresourceRange.baseMipLevel   = 0;
			viewCI.subresourceRange.levelCount     = 1;
			viewCI.subresourceRange.baseArrayLayer = 0;
			viewCI.subresourceRange.layerCount     = 1;
			viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
			viewCI.flags                           = 0;
			viewCI.image                           = pSwapChainImages[i];

			VkImageView imageView;
			VK_CHECK_RESULT(vkCreateImageView(*VulkanDevice::get(), &viewCI, VK_NULL_HANDLE, &imageView));
			auto swapChainBuffer = std::make_shared<VulkanTexture2D>(pSwapChainImages[i], imageView, colorFormat, width, height);
			swapChainBuffer->transitionImage(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			swapChainBuffers.emplace_back(swapChainBuffer);
		}

		delete[] pSwapChainImages;
		createFrameData();
		//createComputeData();

		if (graphicsSemaphore == nullptr)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK_RESULT(vkCreateSemaphore(*VulkanDevice::get(), &semaphoreCreateInfo, nullptr, &graphicsSemaphore));
		}

		// Signal the semaphore
		/*	VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &computeData.semaphore;
		VK_CHECK_RESULT(vkQueueSubmit(VulkanDevice::get()->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK_RESULT(vkQueueWaitIdle(VulkanDevice::get()->getGraphicsQueue()));*/

		return true;
	}

	auto VulkanSwapChain::createFrameData() -> void
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext                 = nullptr;
		semaphoreInfo.flags                 = 0;

		for (uint32_t i = 0; i < swapChainBufferCount; i++)
		{
			if (!frames[i].commandBuffer)
			{
				frames[i].commandPool = std::make_shared<VulkanCommandPool>(VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily.value(),
				                                                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

				frames[i].commandBuffer = std::make_shared<VulkanCommandBuffer>();
				frames[i].commandBuffer->init(true, *frames[i].commandPool);
			}

			if (frames[i].presentSemaphore == nullptr)
				VK_CHECK_RESULT(vkCreateSemaphore(*VulkanDevice::get(), &semaphoreInfo, nullptr, &frames[i].presentSemaphore));
		}
	}

	auto VulkanSwapChain::createComputeData() -> void
	{
		if (computeData.commandBuffer == nullptr)
		{
			computeData.commandPool = std::make_shared<VulkanCommandPool>(
			    VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().computeFamily.value(),
			    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreInfo.pNext                 = nullptr;
			semaphoreInfo.flags                 = 0;

			if (computeData.semaphore == VK_NULL_HANDLE)
				VK_CHECK_RESULT(vkCreateSemaphore(*VulkanDevice::get(), &semaphoreInfo, nullptr, &computeData.semaphore));

			computeData.commandBuffer = std::make_shared<VulkanCommandBuffer>(CommandBufferType::Compute);
			computeData.commandBuffer->init(true, *computeData.commandPool);

			LOGI("Create Vulkan Compute CommandBuffer");
		}
	}

	auto VulkanSwapChain::findImageFormatAndColorSpace() -> void
	{
		PROFILE_FUNCTION();
		VkPhysicalDevice physicalDevice = *VulkanDevice::get()->getPhysicalDevice();

		uint32_t formatCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
		MAPLE_ASSERT(formatCount > 0, "");

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			colorFormat = VK_FORMAT_B8G8R8A8_UNORM;        //use this one as default when there is no surface format
			colorSpace  = surfaceFormats[0].colorSpace;
		}
		else
		{
			bool flag = false;
			for (auto &&surfaceFormat : surfaceFormats)        //VK_FORMAT_B8G8R8A8_UNORM
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					colorFormat = surfaceFormat.format;
					colorSpace  = surfaceFormat.colorSpace;
					flag        = true;
					break;
				}
			}

			// if VK_FORMAT_B8G8R8A8_UNORM is not available, select the first available as default
			if (!flag)
			{
				colorFormat = surfaceFormats[0].format;
				colorSpace  = surfaceFormats[0].colorSpace;
			}
		}
	}

	auto VulkanSwapChain::acquireNextImage() -> void
	{
		PROFILE_FUNCTION();

		//uint32_t nextCmdBufferIndex = (currentBuffer + 1) % swapChainBufferCount;

		if (swapChainBufferCount == 1 && acquireImageIndex != std::numeric_limits<uint32_t>::max())
			return;
		{
			auto &frameData = getFrameData();
			PROFILE_SCOPE("vkAcquireNextImageKHR");
			auto result = vkAcquireNextImageKHR(*VulkanDevice::get(), swapChain, UINT64_MAX, frameData.presentSemaphore, VK_NULL_HANDLE, &acquireImageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				LOGI("Acquire Image result : {0}", result == VK_ERROR_OUT_OF_DATE_KHR ? "Out of Date" : "SubOptimal");

				if (result == VK_ERROR_OUT_OF_DATE_KHR)
				{
					onResize(width, height);
					//acquireNextImage();
				}
				return;
			}
			else if (result != VK_SUCCESS)
			{
				LOGC("[VULKAN] Failed to acquire swap chain image!");
			}
		}
	}

	auto VulkanSwapChain::queueSubmit() -> void
	{
		PROFILE_FUNCTION();
		auto &frameData = getFrameData();
		frameData.commandBuffer->executeInternal(
		    {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		    {frameData.presentSemaphore},
		    {frameData.commandBuffer->getSemaphore()},
		    true);
	}

	auto VulkanSwapChain::begin() -> void
	{
		PROFILE_FUNCTION();
		currentBuffer = (currentBuffer + 1) % swapChainBufferCount;

		auto commandBuffer = getFrameData().commandBuffer;
		if (commandBuffer->getState() == CommandBufferState::Submitted)
		{
			commandBuffer->wait();
		}
		commandBuffer->reset();
		VulkanContext::getDeletionQueue(currentBuffer).flush();
		acquireNextImage();
		commandBuffer->beginRecording();

		/*

		vkQueueWaitIdle(VulkanDevice::get()->getComputeQueue());
		auto cmd = computeData.commandBuffer;
		if (cmd->getState() == CommandBufferState::Submitted)
		{
			cmd->wait();
		}
		cmd->reset();
		cmd->beginRecording();*/
	}

	auto VulkanSwapChain::end() -> void
	{
		PROFILE_FUNCTION();
		getCurrentCommandBuffer()->endRecording();
		//computeData.commandBuffer->endRecording();
	}

	auto VulkanSwapChain::getCurrentCommandBuffer() -> CommandBuffer *
	{
		return getFrameData().commandBuffer.get();
	}

	//swap buffer
	auto VulkanSwapChain::present() -> void
	{
		PROFILE_FUNCTION();

		VkPresentInfoKHR present;
		present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.pNext              = VK_NULL_HANDLE;
		present.swapchainCount     = 1;
		present.pSwapchains        = &swapChain;
		present.pImageIndices      = &acquireImageIndex;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores    = &getFrameData().commandBuffer->getSemaphore();
		present.pResults           = VK_NULL_HANDLE;

		auto error = vkQueuePresentKHR(VulkanDevice::get()->getPresentQueue(), &present);

		if (error == VK_ERROR_OUT_OF_DATE_KHR)
		{
			LOGE("[Vulkan] SwapChain is out of date");
		}
		else if (error == VK_SUBOPTIMAL_KHR)
		{
			LOGE("[Vulkan] SwapChain suboptimal....");
		}
		else
		{
			VK_CHECK_RESULT(error);
		}

		//VK_CHECK_RESULT(vkQueueWaitIdle(VulkanDevice::get()->getGraphicsQueue()));

		/*
		VulkanContext::get()->waitIdle();

		computeData.commandBuffer->executeInternal(
			{ VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },
			{ graphicsSemaphore },
			{ computeData.semaphore },
			true);*/
	}

	auto VulkanSwapChain::getComputeCmdBuffer() -> CommandBuffer *
	{
		return computeData.commandBuffer.get();
	}

	auto VulkanSwapChain::getFrameData() -> FrameData &
	{
		MAPLE_ASSERT(currentBuffer < swapChainBufferCount, "buffer index is out of bounds");
		return frames[currentBuffer];
	}

	auto VulkanSwapChain::onResize(uint32_t width, uint32_t height, bool forceResize /*= false*/, NativeWindow *windowHandle /*= nullptr*/) -> void
	{
		PROFILE_FUNCTION();

		if (!forceResize && this->width == width && this->height == height)
			return;

		this->width  = width;
		this->height = height;

		for (uint32_t i = 0; i < swapChainBufferCount; i++)
		{
			if (frames[i].commandBuffer->getState() == CommandBufferState::Submitted)
				frames[i].commandBuffer->wait();

			swapChainBuffers[i].reset();
		}

		swapChainBuffers.clear();
		oldSwapChain = swapChain;

		swapChain = VK_NULL_HANDLE;

		if (windowHandle)
		{
			init(vsync, windowHandle);
		}
		else
		{
			init(vsync);
		}
	}

};        // namespace maple
