//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanContext.h"
#include "Vk.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanHelper.h"
#include "VulkanRenderDevice.h"
#include "VulkanSwapChain.h"

#ifdef _WIN32
#	include <vulkan/vulkan_win32.h>
#	define GLFW_INCLUDE_VULKAN
#	include "GLFW/glfw3.h"
#endif

#include "Application.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

#define VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME "VK_LAYER_LUNARG_standard_validation"
#define VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME "VK_LAYER_LUNARG_assistant_layer"
#define VK_LAYER_LUNARG_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"
#define VK_LAYER_RENDERDOC_CAPTURE_NAME "VK_LAYER_RENDERDOC_Capture"

namespace maple
{
	struct VkConfig
	{
		static constexpr bool StandardValidationLayer = false;
		static constexpr bool AssistanceLayer         = false;
#ifdef _DEBUG
		static constexpr bool EnableValidationLayers = true;
#else
		static constexpr bool EnableValidationLayers = false;
#endif
	};

	namespace
	{
		VkDebugReportCallbackEXT reportCallback = {};

		inline auto getRequiredLayers()
		{
			std::vector<const char *> layers;
			if constexpr (VkConfig::StandardValidationLayer)
				layers.emplace_back(VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME);

			if constexpr (VkConfig::AssistanceLayer)
				layers.emplace_back(VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME);

			if constexpr (VkConfig::EnableValidationLayers)
			{
				layers.emplace_back(VK_LAYER_LUNARG_VALIDATION_NAME);
			}

			//	layers.emplace_back(VK_LAYER_RENDERDOC_CAPTURE_NAME);
			return layers;
		}

		inline auto getRequiredExtensions()
		{
			std::vector<const char *> extensions;

			if constexpr (VkConfig::EnableValidationLayers)
			{
				extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			extensions.emplace_back("VK_KHR_surface");
#if defined(TRACY_ENABLE) && defined(PLATFORM_WINDOWS)
			//extensions.emplace_back("VK_EXT_calibrated_timestamps");
#endif

#if defined(_WIN32)
			extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
			extensions.emplace_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
			extensions.emplace_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			extensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
			extensions.emplace_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
			extensions.emplace_back("VK_EXT_metal_surface");
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
			extensions.emplace_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
			extensions.emplace_back("VK_EXT_metal_surface");
#endif
			return extensions;
		}

		inline auto checkValidationLayerSupport(std::vector<VkLayerProperties> &layers, std::vector<const char *> &validationLayers)
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			layers.resize(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

			for (const char *layerName : validationLayers)
			{
				bool layerFound = false;
				for (const auto &layerProperties : layers)
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

		inline auto checkExtensionSupport(std::vector<VkExtensionProperties> &properties, std::vector<const char *> &extensions)
		{
			uint32_t extensionCount;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

			properties.resize(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, properties.data());

			bool extensionSupported = true;

			for (int i = 0; i < extensions.size(); i++)
			{
				const char *extensionName = extensions[i];
				bool        layerFound    = false;

				for (const auto &layerProperties : properties)
				{
					if (strcmp(extensionName, layerProperties.extensionName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					extensions.erase(extensions.begin() + i);
					extensionSupported = false;
					LOGW("Extension not supported {0}", extensionName);
				}
			}
			return extensionSupported;
		}

		inline auto debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t sourceObj, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *userData) -> VkBool32
		{
			if (!flags)
				return VK_FALSE;

			if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			{
				LOGW("[VULKAN] - ERROR : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
			};
			if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
			{
				LOGW("[VULKAN] - WARNING : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
			};
			if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			{
				LOGW("[VULKAN] - PERFORMANCE : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
			};
			if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			{
				LOGW("[VULKAN] - INFO : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
			}
			if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			{
				LOGI("[VULKAN] - DEBUG : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
			}
			return VK_FALSE;
		}

		inline auto createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback) -> VkResult
		{
			auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}
	}        // namespace

	VulkanContext::VulkanContext()
	{
	}

	VulkanContext::~VulkanContext()
	{
		for (int32_t i = 0; i < 3; i++)
		{
			getDeletionQueue(i).flush();
		}

		if (reportCallback)
		{
			PFN_vkDestroyDebugReportCallbackEXT destoryCallback = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT");

			destoryCallback(vkInstance, reportCallback, VK_NULL_HANDLE);
		}

		vkDestroyInstance(vkInstance, nullptr);
	}
	/**
	 * setup debug layer
	 */
	auto VulkanContext::setupDebug() -> void
	{
		PROFILE_FUNCTION();
		if constexpr (VkConfig::EnableValidationLayers)
		{
			/*
			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
			createInfo.pfnCallback                        = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(debugCallback);
			if (createDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &reportCallback) != VK_SUCCESS)
			{
				LOGC("[VULKAN] Failed to set up debug callback!");
			}*/

			VkDebugUtilsMessengerCreateInfoEXT createInfo2 = {};
			VulkanHelper::populateDebugMessengerCreateInfo(createInfo2);
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
			func(vkInstance, &createInfo2, nullptr, &debugMessenger);
		}
	}
	/**
	 * init the context
	 */
	auto VulkanContext::init() -> void
	{
		PROFILE_FUNCTION();
		createInstance();
		VulkanDevice::get()->init();
		setupDebug();

		auto &window = Application::getWindow();
		swapChain    = SwapChain::create(window->getWidth(), window->getHeight());
		swapChain->init(false, window.get());
	}

	auto VulkanContext::present() -> void
	{
	}

	auto VulkanContext::getMinUniformBufferOffsetAlignment() const -> size_t
	{
		return VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.minUniformBufferOffsetAlignment;
	}

	auto VulkanContext::alignedDynamicUboSize(size_t size) const -> size_t
	{
		auto min = VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.minUniformBufferOffsetAlignment;

		size_t alignedSize = size;

		if (min > 0)
			alignedSize = (alignedSize + min - 1) & ~(min - 1);

		return alignedSize;
	}

	auto VulkanContext::onImGui() -> void
	{
	}

	auto VulkanContext::waitIdle() const -> void
	{
		vkDeviceWaitIdle(*VulkanDevice::get());
	}

	auto VulkanContext::createInstance() -> void
	{
		PROFILE_FUNCTION();
		instanceLayerNames     = getRequiredLayers();
		instanceExtensionNames = getRequiredExtensions();
		if (VkConfig::EnableValidationLayers && !checkValidationLayerSupport(instanceLayers, instanceLayerNames))
		{
			LOGC("[VULKAN] Validation layers requested, but not available!");
		}

		if (!checkExtensionSupport(instanceExtensions, instanceExtensionNames))
		{
			LOGC("[VULKAN] Extensions requested are not available!");
		}

		instanceExtensionNames.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		VkApplicationInfo appInfo  = {};
		appInfo.pApplicationName   = "Game";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "MapleEngine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo    = {};
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensionNames.size());
		createInfo.ppEnabledExtensionNames = instanceExtensionNames.data();
		createInfo.enabledLayerCount       = static_cast<uint32_t>(instanceLayerNames.size());
		createInfo.ppEnabledLayerNames     = instanceLayerNames.data();

		VkValidationFeatureEnableEXT enabled[] = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
		VkValidationFeaturesEXT      features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
		features.disabledValidationFeatureCount = 0;
		features.enabledValidationFeatureCount  = 1;
		features.pDisabledValidationFeatures    = nullptr;
		features.pEnabledValidationFeatures     = enabled;
		features.pNext                          = nullptr;        //createInfo.pNext;

		createInfo.pNext = &features;

		VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &vkInstance));
	}

	auto VulkanContext::get() -> std::shared_ptr<VulkanContext>
	{
		return std::static_pointer_cast<VulkanContext>(Application::getGraphicsContext());
	}

	auto VulkanContext::getDeletionQueue() -> CommandQueue &
	{
		return get()->deletionQueue[(get()->vkInstance && Application::getWindow()) ? get()->getSwapChain()->getCurrentBufferIndex() : 0];
	}

	auto VulkanContext::getDeletionQueue(uint32_t index) -> CommandQueue &
	{
		MAPLE_ASSERT(index < 3, "Unsupported Frame Index");
		return get()->deletionQueue[index];
	}

	auto VulkanContext::isRaytracingSupported() -> bool
	{
		return VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupported();
	}

	auto VulkanContext::immediateSubmit(const std::function<void(CommandBuffer *)> &execute) -> void
	{
		if (updateFence == nullptr)
		{
			updateFence = std::make_unique<VulkanFence>();
		}

		auto cmd   = CommandBuffer::create();
		auto vkCmd = std::static_pointer_cast<VulkanCommandBuffer>(cmd);
		cmd->init(true);
		cmd->beginRecording();
		execute(cmd.get());
		cmd->endRecording();
		auto         cmdAddress = vkCmd->getCommandBuffer();
		VkSubmitInfo submitInfo{};
		submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount   = 1;
		submitInfo.pCommandBuffers      = &cmdAddress;
		submitInfo.pSignalSemaphores    = nullptr;
		submitInfo.pNext                = nullptr;
		submitInfo.pWaitDstStageMask    = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.waitSemaphoreCount   = 0;

		VK_CHECK_RESULT(vkQueueSubmit(VulkanDevice::get()->getGraphicsQueue(), 1, &submitInfo, updateFence->getHandle()));
		updateFence->waitAndReset();
	}
}        // namespace maple
