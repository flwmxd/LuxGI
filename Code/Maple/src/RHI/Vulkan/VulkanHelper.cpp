//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanHelper.h"
#include "Engine/Vertex.h"
#include "Others/Console.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanContext.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

#include <string>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Application.h"
namespace maple
{
	namespace
	{
		inline auto isDepthFormat(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
					return true;
			}
			return false;
		}

		inline auto isStencilFormat(VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
					return true;
			}
			return false;
		}

		inline auto layoutToAccessMask(const VkImageLayout layout, const bool isDestination) -> VkPipelineStageFlags
		{
			VkPipelineStageFlags accessMask = 0;

			switch (layout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					if (isDestination)
					{
						LOGE("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED.");
					}
					break;

				case VK_IMAGE_LAYOUT_GENERAL:
					accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
					accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					accessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					if (!isDestination)
					{
						accessMask = VK_ACCESS_HOST_WRITE_BIT;
					}
					else
					{
						LOGE("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED.");
					}
					break;

				case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
					accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
					accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
					accessMask = VK_ACCESS_MEMORY_READ_BIT;
					break;

				default:
					LOGE("Unexpected image layout");
					break;
			}

			return accessMask;
		}

		inline auto accessFlagsToPipelineStage(VkAccessFlags accessFlags, const VkPipelineStageFlags stageFlags)
		{
			VkPipelineStageFlags stages = 0;

			while (accessFlags != 0)
			{
				VkAccessFlagBits newAccessFlag = static_cast<VkAccessFlagBits>(accessFlags & (~(accessFlags - 1)));
				MAPLE_ASSERT(newAccessFlag != 0 && (newAccessFlag & (newAccessFlag - 1)) == 0, "Error in access flags");
				accessFlags &= ~newAccessFlag;

				switch (newAccessFlag)
				{
					case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
						stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
						break;

					case VK_ACCESS_INDEX_READ_BIT:
						stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						break;

					case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
						stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						break;

					case VK_ACCESS_UNIFORM_READ_BIT:
						stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						break;

					case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
						stages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
						break;

					case VK_ACCESS_SHADER_READ_BIT:
						stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						break;

					case VK_ACCESS_SHADER_WRITE_BIT:
						stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
						break;

					case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
						stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						break;

					case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
						stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						break;

					case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
						stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						break;

					case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
						stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						break;

					case VK_ACCESS_TRANSFER_READ_BIT:
						stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
						break;

					case VK_ACCESS_TRANSFER_WRITE_BIT:
						stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
						break;

					case VK_ACCESS_HOST_READ_BIT:
						stages |= VK_PIPELINE_STAGE_HOST_BIT;
						break;

					case VK_ACCESS_HOST_WRITE_BIT:
						stages |= VK_PIPELINE_STAGE_HOST_BIT;
						break;

					case VK_ACCESS_MEMORY_READ_BIT:
						break;

					case VK_ACCESS_MEMORY_WRITE_BIT:
						break;

					default:
						LOGE("Unknown access flag");
						break;
				}
			}
			return stages;
		}
	}        // namespace

	auto VulkanHelper::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) -> uint32_t
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(*VulkanDevice::get()->getPhysicalDevice(), &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
			    (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	auto VulkanHelper::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) -> SwapChainSupportDetails
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	auto VulkanHelper::checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &deviceExtensions) -> bool
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto &extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	auto VulkanHelper::checkValidationLayerSupport(const std::vector<const char *> &layerNames) -> bool
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (auto layerName : layerNames)
		{
			bool layerFound = false;

			for (const auto &layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}
		return true;
	}

	auto VulkanHelper::copyTo(const Texture2D::Ptr &from, Texture2D::Ptr &to, const CommandBuffer *cmdBuffer) -> void
	{
		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		const auto vkFrom = std::static_pointer_cast<VulkanTexture2D>(from);
		auto       vkTo   = std::static_pointer_cast<VulkanTexture2D>(to);

		vkFrom->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<const VulkanCommandBuffer *>(cmdBuffer));
		vkTo->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<const VulkanCommandBuffer *>(cmdBuffer));

		auto vkFromLayout = vkFrom->getImageLayout();
		auto vkToLayout   = vkTo->getImageLayout();

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width              = from->getWidth();
		imageCopyRegion.extent.height             = from->getHeight();
		imageCopyRegion.extent.depth              = 1;

		// Issue the copy command
		vkCmdCopyImage(
		    static_cast<const VulkanCommandBuffer *>(cmdBuffer)->getCommandBuffer(),
		    vkFrom->getImage(),
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    vkTo->getImage(),
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &imageCopyRegion);

		vkFrom->transitionImage(vkFromLayout, static_cast<const VulkanCommandBuffer *>(cmdBuffer));
		vkTo->transitionImage(vkToLayout, static_cast<const VulkanCommandBuffer *>(cmdBuffer));
	}

	auto VulkanHelper::validateResolution(uint32_t &width, uint32_t &height) -> void
	{
		VkSurfaceCapabilitiesKHR capabilities;
		auto                     surface = std::static_pointer_cast<VulkanSwapChain>(Application::getGraphicsContext()->getSwapChain())->getSurface();
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*VulkanDevice::get()->getPhysicalDevice(), surface, &capabilities);

		width  = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
		height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
	}

	auto VulkanHelper::instanceCreateInfo(const VkApplicationInfo &appInfo, const std::vector<const char *> &extensions, const std::vector<const char *> &validationLayers, bool validationLayer) -> VkInstanceCreateInfo
	{
		//Create Info
		VkInstanceCreateInfo createInfo    = {};
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.pNext                   = NULL;
		createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (validationLayer)
		{
			createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext             = nullptr;
		}

		return createInfo;
	}

	auto VulkanHelper::getApplicationInfo(const std::string &name) -> VkApplicationInfo
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName  = name.c_str();
		appInfo.pEngineName       = name.c_str();
		appInfo.apiVersion        = VK_API_VERSION_1_2;
		appInfo.engineVersion     = VK_MAKE_VERSION(1, 0, 0);
		return appInfo;
	}

	auto VulkanHelper::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) -> QueueFamilyIndices
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				indices.computeFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		return indices;
	}

	auto VulkanHelper::createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory, VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBufferCreateFlags flags /*= 0*/, VkSharingMode sharingMode /*= VK_SHARING_MODE_EXCLUSIVE*/, const std::vector<uint32_t> &queueFamilyIndices /*= {}*/) -> void
	{
		VkBufferCreateInfo bufferInfo    = {};
		bufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size                  = size;
		bufferInfo.usage                 = usage;
		bufferInfo.sharingMode           = sharingMode;
		bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		bufferInfo.pQueueFamilyIndices   = queueFamilyIndices.size() == 0 ? nullptr : queueFamilyIndices.data();
		bufferInfo.flags                 = flags;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize       = memRequirements.size;
		allocInfo.memoryTypeIndex      = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	auto VulkanHelper::physicalDeviceTypeString(VkPhysicalDeviceType type) -> std::string
	{
		switch (type)
		{
#define STR(r)                        \
	case VK_PHYSICAL_DEVICE_TYPE_##r: \
		return #r
			STR(OTHER);
			STR(INTEGRATED_GPU);
			STR(DISCRETE_GPU);
			STR(VIRTUAL_GPU);
#undef STR
			default:
				return "UNKNOWN_DEVICE_TYPE";
		}
	}

	auto VulkanHelper::errorString(VkResult errorCode) -> std::string
	{
		switch (errorCode)
		{
#define STR(r)   \
	case VK_##r: \
		return #r
			STR(NOT_READY);
			STR(TIMEOUT);
			STR(EVENT_SET);
			STR(EVENT_RESET);
			STR(INCOMPLETE);
			STR(ERROR_OUT_OF_HOST_MEMORY);
			STR(ERROR_OUT_OF_DEVICE_MEMORY);
			STR(ERROR_INITIALIZATION_FAILED);
			STR(ERROR_DEVICE_LOST);
			STR(ERROR_MEMORY_MAP_FAILED);
			STR(ERROR_LAYER_NOT_PRESENT);
			STR(ERROR_EXTENSION_NOT_PRESENT);
			STR(ERROR_FEATURE_NOT_PRESENT);
			STR(ERROR_INCOMPATIBLE_DRIVER);
			STR(ERROR_TOO_MANY_OBJECTS);
			STR(ERROR_FORMAT_NOT_SUPPORTED);
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}

	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	auto VulkanHelper::setupDebugging(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportCallbackEXT callBack) -> void
	{
		auto vkCreateDebugUtilsMessengerEXT  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
		debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCI.messageSeverity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCI.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessengerCallback;
		VkResult result                       = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
	}

	auto VulkanHelper::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) -> void
	{
		createInfo       = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugUtilsMessengerCallback;
	}

	auto VulkanHelper::getSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) -> VkFormat
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(*VulkanDevice::get()->getPhysicalDevice(), format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	auto VulkanHelper::getDepthFormat(bool stencil) -> VkFormat
	{
		if (stencil)
		{
			return getSupportedFormat(
			    {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT},
			    VK_IMAGE_TILING_OPTIMAL,
			    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}
		return getSupportedFormat(
		    {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT},
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	auto VulkanHelper::setObjectName(const std::string &name, uint64_t handle, VkObjectType objType) -> void
	{
		if (name != "" && handle != 0)
		{
			VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objType, handle, name.c_str()};
			vkSetDebugUtilsObjectNameEXT(*VulkanDevice::get(), &s);
		}
	}

	auto VulkanHelper::getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat) -> VkBool32
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<VkFormat> depthFormats = {
		    VK_FORMAT_D32_SFLOAT_S8_UINT,
		    VK_FORMAT_D32_SFLOAT,
		    VK_FORMAT_D24_UNORM_S8_UINT,
		    VK_FORMAT_D16_UNORM_S8_UINT,
		    VK_FORMAT_D16_UNORM};

		for (auto &format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				*depthFormat = format;
				return true;
			}
		}
		return false;
	}

	auto VulkanHelper::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) -> VkSurfaceFormatKHR
	{
		for (const auto &availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

	auto VulkanHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes, bool vsync) -> VkPresentModeKHR
	{
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!vsync)
		{
			for (size_t i = 0; i < availablePresentModes.size(); i++)
			{
				if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if (availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}
		return swapchainPresentMode;
	}

	auto VulkanHelper::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}

		int32_t width, height;
		glfwGetFramebufferSize(static_cast<GLFWwindow *>(Application::get()->getWindow()->getNativeInterface()), &width, &height);
		VkExtent2D actualExtent = {
		    static_cast<uint32_t>(width),
		    static_cast<uint32_t>(height)};
		actualExtent.width  = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}

	auto VulkanHelper::createImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageViewType viewType, VkImageAspectFlags aspectMask, uint32_t layerCount, uint32_t baseArrayLayer, uint32_t baseMipLevel) -> VkImageView
	{
		PROFILE_FUNCTION();
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                 = image;
		viewInfo.viewType              = viewType;
		viewInfo.format                = format;
#ifdef PLATFORM_MACOS
		viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
#else
		viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
#endif
		viewInfo.subresourceRange.aspectMask     = aspectMask;
		viewInfo.subresourceRange.baseMipLevel   = baseMipLevel;
		viewInfo.subresourceRange.levelCount     = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
		viewInfo.subresourceRange.layerCount     = layerCount;

		VkImageView imageView;
		if (vkCreateImageView(*VulkanDevice::get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			LOGE("Failed to create texture image view!");
		}

		return imageView;
	}

#ifdef USE_VMA_ALLOCATOR
	auto VulkanHelper::createImageVma(const VkImageCreateInfo &imageInfo, VkImage &image, VmaAllocation &allocation) -> void
	{
		PROFILE_FUNCTION();
		VmaAllocationCreateInfo allocInfovma;
		allocInfovma.flags          = 0;
		allocInfovma.usage          = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfovma.requiredFlags  = 0;
		allocInfovma.preferredFlags = 0;
		allocInfovma.memoryTypeBits = 0;
		allocInfovma.pool           = nullptr;
		allocInfovma.pUserData      = nullptr;
		VK_CHECK_RESULT(vmaCreateImage(VulkanDevice::get()->getAllocator(), &imageInfo, &allocInfovma, &image, &allocation, nullptr));
	}

#else
	auto VulkanHelper::createImageDefault(const VkImageCreateInfo &imageInfo, VkImage &image, VkDeviceMemory &imageMemory, VkMemoryPropertyFlags properties) -> void
	{
		PROFILE_FUNCTION();
		vkCreateImage(*VulkanDevice::get(), &imageInfo, nullptr, &image);

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(*VulkanDevice::get(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanHelper::findMemoryType(memRequirements.memoryTypeBits, properties);

		vkAllocateMemory(*VulkanDevice::get(), &allocInfo, nullptr, &imageMemory);
		vkBindImageMemory(*VulkanDevice::get(), image, imageMemory, 0);
	}
#endif

#ifdef USE_VMA_ALLOCATOR
	auto VulkanHelper::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, VmaAllocation &allocation, uint32_t depth, VkImageLayout initLayout, void *next) -> void
#else
	auto VulkanHelper::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, uint32_t depth, VkImageLayout initLayout, void *next) -> void
#endif
	{
		PROFILE_FUNCTION();
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType         = imageType;
		imageInfo.extent            = {width, height, depth};
		imageInfo.mipLevels         = mipLevels;
		imageInfo.format            = format;
		imageInfo.tiling            = tiling;
		imageInfo.initialLayout     = initLayout;
		imageInfo.usage             = usage;
		imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.arrayLayers       = arrayLayers;
		imageInfo.flags             = flags;
		imageInfo.pNext             = next;

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImageVma(imageInfo, image, allocation);
#else
		VulkanHelper::createImageDefault(imageInfo, image, imageMemory, properties);
#endif
	}

	auto VulkanHelper::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount,
	                                         const VulkanCommandBuffer *cmd, bool depth, uint32_t baseArrayLayer, uint32_t baseMipLevel) -> void
	{
		PROFILE_FUNCTION();

		auto commandBuffer = cmd != nullptr ? cmd->getCommandBuffer() : nullptr;

		bool singleTimeCommand = false;

		if (!commandBuffer)
		{
			commandBuffer     = beginSingleTimeCommands();
			singleTimeCommand = true;
		}

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask              = depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		if (isStencilFormat(format))
			subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		subresourceRange.baseMipLevel   = baseMipLevel;
		subresourceRange.baseArrayLayer = baseArrayLayer;
		subresourceRange.levelCount     = mipLevels;
		subresourceRange.layerCount     = layerCount;

		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = VulkanHelper::imageMemoryBarrier();
		imageMemoryBarrier.oldLayout            = oldLayout;
		imageMemoryBarrier.newLayout            = newLayout;
		imageMemoryBarrier.image                = image;
		imageMemoryBarrier.subresourceRange     = subresourceRange;
		imageMemoryBarrier.srcAccessMask        = layoutToAccessMask(oldLayout, false);
		imageMemoryBarrier.dstAccessMask        = layoutToAccessMask(newLayout, true);

		if (cmd != nullptr && cmd->getCommandBuffeType() == CommandBufferType::Compute)
		{
			imageMemoryBarrier.srcQueueFamilyIndex = VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().computeFamily.value();
			imageMemoryBarrier.dstQueueFamilyIndex = VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().computeFamily.value();
		}
		else
		{
			imageMemoryBarrier.srcQueueFamilyIndex = VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily.value();
			imageMemoryBarrier.dstQueueFamilyIndex = VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily.value();
		}

		vkCmdPipelineBarrier(
		    commandBuffer,
		    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		    0,
		    0, nullptr,
		    0, nullptr,
		    1, &imageMemoryBarrier);

		if (singleTimeCommand)
			endSingleTimeCommands(commandBuffer);
	}

	auto VulkanHelper::beginSingleTimeCommands() -> VkCommandBuffer
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool        = *VulkanDevice::get()->getCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(*VulkanDevice::get(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		return commandBuffer;
	}

	auto VulkanHelper::endSingleTimeCommands(VkCommandBuffer commandBuffer) -> void
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount   = 1;
		submitInfo.pCommandBuffers      = &commandBuffer;
		submitInfo.pSignalSemaphores    = nullptr;
		submitInfo.pNext                = nullptr;
		submitInfo.pWaitDstStageMask    = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.waitSemaphoreCount   = 0;

		VK_CHECK_RESULT(vkQueueSubmit(VulkanDevice::get()->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK_RESULT(vkQueueWaitIdle(VulkanDevice::get()->getGraphicsQueue()));

		vkFreeCommandBuffers(*VulkanDevice::get(), *VulkanDevice::get()->getCommandPool(), 1, &commandBuffer);
	}

	auto VulkanHelper::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth, int32_t offsetX, int32_t offsetY, int32_t offsetZ, const VulkanCommandBuffer *cmd) -> void
	{
		VkCommandBuffer commandBuffer = cmd == nullptr ? beginSingleTimeCommands() : cmd->getCommandBuffer();

		VkBufferImageCopy region;
		region.bufferOffset                    = 0;
		region.bufferRowLength                 = 0;
		region.bufferImageHeight               = 0;
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount     = 1;
		region.imageOffset                     = {offsetX, offsetY, offsetZ};
		region.imageExtent                     = {
            width,
            height,
            depth};
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		if (cmd == nullptr)
			endSingleTimeCommands(commandBuffer);
	}

	VKAPI_ATTR auto VKAPI_CALL VulkanHelper::debugUtilsMessengerCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT             messageType,
	    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	    void *                                      pUserData) -> VkBool32
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				LOGV("validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				LOGW("validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				LOGI("validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				LOGE("validation layer: {0}", pCallbackData->pMessage);
				break;
		}

		//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	auto VulkanHelper::setImageLayout(
	    VkCommandBuffer         cmdbuffer,
	    VkImage                 image,
	    VkImageLayout           oldImageLayout,
	    VkImageLayout           newImageLayout,
	    VkImageSubresourceRange subresourceRange,
	    VkPipelineStageFlags    srcStageMask,
	    VkPipelineStageFlags    dstStageMask) -> void
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = VulkanHelper::imageMemoryBarrier();
		imageMemoryBarrier.oldLayout            = oldImageLayout;
		imageMemoryBarrier.newLayout            = newImageLayout;
		imageMemoryBarrier.image                = image;
		imageMemoryBarrier.subresourceRange     = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
		    cmdbuffer,
		    srcStageMask,
		    dstStageMask,
		    0,
		    0, nullptr,
		    0, nullptr,
		    1, &imageMemoryBarrier);
	}

	// Fixed sub resource on first mip level and layer
	auto VulkanHelper::setImageLayout(
	    VkCommandBuffer      cmdbuffer,
	    VkImage              image,
	    VkImageAspectFlags   aspectMask,
	    VkImageLayout        oldImageLayout,
	    VkImageLayout        newImageLayout,
	    VkPipelineStageFlags srcStageMask,
	    VkPipelineStageFlags dstStageMask) -> void
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask              = aspectMask;
		subresourceRange.baseMipLevel            = 0;
		subresourceRange.levelCount              = 1;
		subresourceRange.layerCount              = 1;
		setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
	}

	auto VulkanHelper::createTextureSampler(VkFilter magFilter /*= VK_FILTER_LINEAR*/, VkFilter minFilter /*= VK_FILTER_LINEAR*/, float minLod /*= 0.0f*/, float maxLod /*= 1.0f*/, bool anisotropyEnable /*= false*/, float maxAnisotropy /*= 1.0f*/, VkSamplerAddressMode modeU /*= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE*/, VkSamplerAddressMode modeV /*= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE*/, VkSamplerAddressMode modeW /*= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE*/) -> VkSampler
	{
		PROFILE_FUNCTION();
		VkSampler           sampler;
		VkSamplerCreateInfo samplerInfo     = {};
		samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter               = magFilter;
		samplerInfo.minFilter               = minFilter;
		samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU            = modeU;
		samplerInfo.addressModeV            = modeV;
		samplerInfo.addressModeW            = modeW;
		samplerInfo.maxAnisotropy           = maxAnisotropy;
		samplerInfo.anisotropyEnable        = anisotropyEnable;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable           = VK_FALSE;
		samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.mipLodBias              = 0.0f;
		samplerInfo.compareOp               = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod                  = minLod;
		samplerInfo.maxLod                  = maxLod;
		VK_CHECK_RESULT(vkCreateSampler(*VulkanDevice::get(), &samplerInfo, nullptr, &sampler));
		return sampler;
	}

	namespace VkConverter
	{
		auto textureFormatToVK(const TextureFormat &format, bool srgb, bool ignoreAssert) -> VkFormat
		{
			if (srgb)
			{
				switch (format)
				{
					case TextureFormat::RGBA:
						return VK_FORMAT_R8G8B8A8_UNORM;
					case TextureFormat::RGB:
						return VK_FORMAT_R8G8B8_SRGB;
					case TextureFormat::R8:
						return VK_FORMAT_R8_SRGB;
					case TextureFormat::RG8:
						return VK_FORMAT_R8G8_SRGB;
					case TextureFormat::RGB8:
						return VK_FORMAT_R8G8B8A8_SRGB;
					case TextureFormat::RGBA8:
						return VK_FORMAT_R8G8B8A8_SRGB;
					case TextureFormat::RGB16:
						return VK_FORMAT_R16G16B16_SFLOAT;
					case TextureFormat::RGBA16:
						return VK_FORMAT_R16G16B16A16_SFLOAT;
					case TextureFormat::RGB32:
						return VK_FORMAT_R32G32B32_SFLOAT;
					case TextureFormat::RGBA32:
						return VK_FORMAT_R32G32B32A32_SFLOAT;
					case TextureFormat::R32F:
						return VK_FORMAT_R32_SFLOAT;
					case TextureFormat::R16F:
						return VK_FORMAT_R16_SFLOAT;
					default:
						MAPLE_ASSERT(ignoreAssert, "[Texture] Unsupported image bit-depth!");
						return VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				switch (format)
				{
					case TextureFormat::RGBA:
						return VK_FORMAT_R8G8B8A8_UNORM;
					case TextureFormat::RGB:
						return VK_FORMAT_R8G8B8_UNORM;
					case TextureFormat::R8:
						return VK_FORMAT_R8_UNORM;
					case TextureFormat::R16:
						return VK_FORMAT_R16_SFLOAT;
					case TextureFormat::RG8:
						return VK_FORMAT_R8G8_UNORM;
					case TextureFormat::RGB8:
						return VK_FORMAT_R8G8B8A8_UNORM;
					case TextureFormat::RGBA8:
						return VK_FORMAT_R8G8B8A8_UNORM;
					case TextureFormat::RGB16:
						return VK_FORMAT_R16G16B16_SFLOAT;
					case TextureFormat::RGBA16:
						return VK_FORMAT_R16G16B16A16_SFLOAT;
					case TextureFormat::RGB32:
						return VK_FORMAT_R32G32B32_SFLOAT;
					case TextureFormat::RGBA32:
						return VK_FORMAT_R32G32B32A32_SFLOAT;
					case TextureFormat::R32UI:
						return VK_FORMAT_R32_UINT;
					case TextureFormat::R32I:
						return VK_FORMAT_R32_SINT;
					case TextureFormat::RG16F:
						return VK_FORMAT_R16G16_SFLOAT;
					case TextureFormat::R32F:
						return VK_FORMAT_R32_SFLOAT;
					case TextureFormat::R16F:
						return VK_FORMAT_R16_SFLOAT;
					case TextureFormat::R11G11B10:
						return VK_FORMAT_R16G16B16A16_SFLOAT;
					default:
						MAPLE_ASSERT(ignoreAssert, "[Texture] Unsupported image bit-depth!");
						return VK_FORMAT_UNDEFINED;
				}
			}
		}

		auto textureFilterToVK(TextureFilter filter) -> VkFilter
		{
			switch (filter)
			{
				case TextureFilter::Nearest:
					return VK_FILTER_NEAREST;
				case TextureFilter::Linear:
					return VK_FILTER_LINEAR;
				case TextureFilter::None:
					return VK_FILTER_LINEAR;
				default:
					LOGC("[Texture] Unsupported TextureFilter type!");
					return VK_FILTER_LINEAR;
			}
		}

		auto textureWrapToVK(TextureWrap wrap) -> VkSamplerAddressMode
		{
			switch (wrap)
			{
				case TextureWrap::Clamp:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				case TextureWrap::ClampToBorder:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				case TextureWrap::ClampToEdge:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				case TextureWrap::Repeat:
					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
				case TextureWrap::MirroredRepeat:
					return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
				default:
					LOGC("[Texture] Unsupported wrap type!");
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		}
		auto drawTypeToTopology(DrawType type) -> VkPrimitiveTopology
		{
			switch (type)
			{
				case DrawType::Triangle:
					return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case DrawType::Lines:
					return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				case DrawType::Point:
					return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				case DrawType::TriangleStrip:
					return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				case DrawType::Lines_Strip:
					return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				default:
					LOGC("Unknown Draw Type");
					return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			}
		}
		auto cullModeToVk(CullMode mode) -> VkCullModeFlags
		{
			switch (mode)
			{
				case CullMode::Back:
					return VK_CULL_MODE_BACK_BIT;
				case CullMode::Front:
					return VK_CULL_MODE_FRONT_BIT;
				case CullMode::FrontAndBack:
					return VK_CULL_MODE_FRONT_AND_BACK;
				case CullMode::None:
					return VK_CULL_MODE_NONE;
			}

			return VK_CULL_MODE_BACK_BIT;
		}
		auto polygonModeToVk(PolygonMode mode) -> VkPolygonMode
		{
			switch (mode)
			{
				case PolygonMode::Fill:
					return VK_POLYGON_MODE_FILL;
					break;
				case PolygonMode::Line:
					return VK_POLYGON_MODE_LINE;
					break;
				case PolygonMode::Point:
					return VK_POLYGON_MODE_POINT;
					break;
				default:
					LOGC("Unknown Polygon Mode");
					return VK_POLYGON_MODE_FILL;
					break;
			}
		}

		auto descriptorTypeToVK(DescriptorType type) -> VkDescriptorType
		{
			switch (type)
			{
				case DescriptorType::UniformBuffer:
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				case DescriptorType::UniformBufferDynamic:
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				case DescriptorType::ImageSampler:
					return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				case DescriptorType::Image:
					return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				case DescriptorType::Buffer:
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				case DescriptorType::BufferDynamic:
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				case DescriptorType::AccelerationStructure:
					return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			}
			MAPLE_ASSERT(false, "Unknown DescriptorType");
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}

		auto shaderTypeToVK(const ShaderType &shaderName) -> VkShaderStageFlagBits
		{
			constexpr VkShaderStageFlagBits types[12] =
			    {
			        VK_SHADER_STAGE_VERTEX_BIT,
			        VK_SHADER_STAGE_FRAGMENT_BIT,
			        VK_SHADER_STAGE_GEOMETRY_BIT,
			        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
			        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
			        VK_SHADER_STAGE_COMPUTE_BIT,
			        VK_SHADER_STAGE_MISS_BIT_KHR,
			        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			        VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			        VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
			        VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM};

			uint32_t retBits = 0;

			const ShaderType flags[11] = {
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
			    ShaderType::RayIntersect};

			for (auto i = 0; i < 11; i++)
			{
				if (((uint32_t) shaderName & (uint32_t) flags[i]) == (uint32_t) flags[i])
				{
					retBits |= types[i];
				}
			}

			if (retBits != 0)
				return (VkShaderStageFlagBits) retBits;

			LOGC("Unknown Shader Type");
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}        // namespace VkConverter
};           // namespace maple
