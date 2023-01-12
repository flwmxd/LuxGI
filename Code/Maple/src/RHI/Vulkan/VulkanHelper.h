//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RHI/Definitions.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Vk.h"
#include <array>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#define autoUnpack(x)            \
	inline operator auto() const \
	{                            \
		return x;                \
	}

#define VK_CHECK_RESULT(f)                                                                                                        \
	{                                                                                                                             \
		VkResult res = (f);                                                                                                       \
		if (res != VK_SUCCESS)                                                                                                    \
		{                                                                                                                         \
			LOGE("Fatal : VkResult is \" {0} \" in {1} at line {2}", VulkanHelper::errorString(res).c_str(), __FILE__, __LINE__); \
			assert(res == VK_SUCCESS);                                                                                            \
		}                                                                                                                         \
	}
namespace maple
{
	enum class DescriptorType;
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR        capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>   presentModes;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> computeFamily;
		auto                    isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
		}
	};

	class CommandBuffer;

	namespace VulkanHelper
	{
		auto validateResolution(uint32_t &width, uint32_t &height) -> void;
		auto instanceCreateInfo(const VkApplicationInfo &appInfo, const std::vector<const char *> &extensions, const std::vector<const char *> &validationLayers, bool validationLayer) -> VkInstanceCreateInfo;
		auto getApplicationInfo(const std::string &name) -> VkApplicationInfo;
		auto findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) -> QueueFamilyIndices;
		auto findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) -> uint32_t;
		auto querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) -> SwapChainSupportDetails;
		auto checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &deviceExtensions) -> bool;
		auto checkValidationLayerSupport(const std::vector<const char *> &layerNames) -> bool;

		auto copyTo(const Texture2D::Ptr &from, Texture2D::Ptr &to,const CommandBuffer * cmdBuffer) -> void;


#ifdef USE_VMA_ALLOCATOR
		auto createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, VmaAllocation &allocation, uint32_t depth = 1, VkImageLayout initLayout = VK_IMAGE_LAYOUT_UNDEFINED, void *next = nullptr) -> void;
#else
		auto createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, uint32_t depth = 1, VkImageLayout initLayout = VK_IMAGE_LAYOUT_UNDEFINED, void *next = nullptr) -> void;
#endif        // USE_VMA_ALLOCATOR

		auto createImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageViewType viewType, VkImageAspectFlags aspectMask, uint32_t layerCount, uint32_t baseArrayLayer = 0, uint32_t baseMipLevel = 0) -> VkImageView;

		auto createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory,
		                  VkPhysicalDevice physicalDevice, VkDevice device,
		                  VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBufferCreateFlags flags = 0,
		                  VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, const std::vector<uint32_t> &queueFamilyIndices = {}) -> void;

		// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
		auto setImageLayout(
		    VkCommandBuffer         cmdbuffer,
		    VkImage                 image,
		    VkImageLayout           oldImageLayout,
		    VkImageLayout           newImageLayout,
		    VkImageSubresourceRange subresourceRange,
		    VkPipelineStageFlags    srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		    VkPipelineStageFlags    dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) -> void;
		// Uses a fixed sub resource layout with first mip level and layer
		auto setImageLayout(
		    VkCommandBuffer      cmdbuffer,
		    VkImage              image,
		    VkImageAspectFlags   aspectMask,
		    VkImageLayout        oldImageLayout,
		    VkImageLayout        newImageLayout,
		    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) -> void;

		auto physicalDeviceTypeString(VkPhysicalDeviceType type) -> std::string;
		auto errorString(VkResult errorCode) -> std::string;

		auto setupDebugging(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportCallbackEXT callBack) -> void;

		auto populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) -> void;

		VKAPI_ATTR auto VKAPI_CALL debugUtilsMessengerCallback(
		    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
		    VkDebugUtilsMessageTypeFlagsEXT             messageType,
		    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		    void *                                      pUserData) -> VkBool32;

		auto getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat) -> VkBool32;
		auto getSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) -> VkFormat;
		auto getDepthFormat(bool stencil = false) -> VkFormat;

		auto setObjectName(const std::string &name, uint64_t handle, VkObjectType objType) -> void;

		inline auto semaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			return semaphoreCreateInfo;
		}

		inline auto submitInfo()
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			return submitInfo;
		}

		inline auto bufferCreateInfo()
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			return bufCreateInfo;
		}

		inline auto bufferCreateInfo(
		    VkBufferUsageFlags usage,
		    VkDeviceSize       size)
		{
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.usage = usage;
			bufCreateInfo.size  = size;
			return bufCreateInfo;
		}

		inline auto memoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		inline auto mappedMemoryRange()
		{
			VkMappedMemoryRange mappedMemoryRange{};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemoryRange;
		}

		inline auto imageCreateInfo()
		{
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return imageCreateInfo;
		}

		/** @brief Initialize an image memory barrier with no image transfer ownership */
		inline auto imageMemoryBarrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imageMemoryBarrier;
		}

		inline auto commandBufferAllocateInfo(
		    VkCommandPool        commandPool,
		    VkCommandBufferLevel level,
		    uint32_t             bufferCount)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool        = commandPool;
			commandBufferAllocateInfo.level              = level;
			commandBufferAllocateInfo.commandBufferCount = bufferCount;
			return commandBufferAllocateInfo;
		}

		inline auto commandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo{};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			return cmdPoolCreateInfo;
		}

		inline auto commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			return cmdBufferBeginInfo;
		}

		inline auto renderPassCreateInfo()
		{
			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			return renderPassCreateInfo;
		}

		inline auto fenceCreateInfo(VkFenceCreateFlags flags = 0)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = flags;
			return fenceCreateInfo;
		}

		inline auto hasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		inline VkPushConstantRange pushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = stageFlags;
			pushConstantRange.offset     = offset;
			pushConstantRange.size       = size;
			return pushConstantRange;
		}

		auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) -> VkSurfaceFormatKHR;
		auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes, bool vsync) -> VkPresentModeKHR;
		auto chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D;

		auto transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
		                           uint32_t mipLevels = 1, uint32_t layerCount = 1, const VulkanCommandBuffer *cmd = nullptr,
		                           bool depth = true, uint32_t baseArrayLayer = 0, uint32_t baseMipLevel = 0) -> void;

		auto copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth = 1, int32_t offsetX = 0, int32_t offsetY = 0, int32_t offsetZ = 0, const VulkanCommandBuffer * cmd = nullptr) -> void;
		auto createTextureSampler(VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, float minLod = 0.0f, float maxLod = 1.0f, bool anisotropyEnable = false, float maxAnisotropy = 1.0f, VkSamplerAddressMode modeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkSamplerAddressMode modeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkSamplerAddressMode modeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) -> VkSampler;
		auto beginSingleTimeCommands() -> VkCommandBuffer;
		auto endSingleTimeCommands(VkCommandBuffer commandBuffer) -> void;
#ifdef USE_VMA_ALLOCATOR
		auto createImageVma(const VkImageCreateInfo &imageInfo, VkImage &image, VmaAllocation &allocation) -> void;
#else
		auto createImageDefault(const VkImageCreateInfo &imageInfo, VkImage &image, VkDeviceMemory &imageMemory, VkMemoryPropertyFlags properties) -> void;
#endif
	};        // namespace VulkanHelper

	namespace VkConverter
	{
		auto textureFormatToVK(const TextureFormat &format, bool srgb = false, bool ignoreAssert = false) -> VkFormat;
		auto textureFilterToVK(TextureFilter filter) -> VkFilter;
		auto textureWrapToVK(TextureWrap wrap) -> VkSamplerAddressMode;
		auto drawTypeToTopology(DrawType type) -> VkPrimitiveTopology;
		auto cullModeToVk(CullMode cull) -> VkCullModeFlags;
		auto polygonModeToVk(PolygonMode mode) -> VkPolygonMode;
		auto descriptorTypeToVK(DescriptorType type) -> VkDescriptorType;
		auto shaderTypeToVK(const ShaderType &shaderName) -> VkShaderStageFlagBits;
	};        // namespace VkConverter
};            // namespace maple
