//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "VulkanHelper.h"

#include <TracyVulkan.hpp>
#include <assert.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
namespace maple
{
	class VulkanCommandPool;

	class VulkanPhysicalDevice final
	{
	  public:
		VulkanPhysicalDevice();
		NO_COPYABLE(VulkanPhysicalDevice);
		inline auto isExtensionSupported(const std::string &extensionName) const
		{
			return supportedExtensions.find(extensionName) != supportedExtensions.end();
		}

		auto getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const -> uint32_t;

		inline operator auto() const
		{
			return physicalDevice;
		}

		inline auto &getProperties() const
		{
			return physicalDeviceProperties;
		};

		inline auto &getQueueFamilyIndices() const
		{
			return indices;
		}
		inline auto &getMemoryProperties() const
		{
			return memoryProperties;
		}

		auto getRaytracingProperties() -> void;

		inline auto &getRayTracingPipelineProperties() const
		{
			return rayTracingPipelineProperties;
		}
		inline auto &getAccelerationStructureProperties() const
		{
			return accelerationStructureProperties;
		}

		inline auto isRaytracingSupported() const
		{
			return raytracingSupport;
		}

	  private:
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		std::unordered_set<std::string>      supportedExtensions;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		VkPhysicalDevice                                   physicalDevice;
		VkPhysicalDeviceProperties                         physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties                   memoryProperties;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR    rayTracingPipelineProperties;
		VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;

		friend class VulkanDevice;
		QueueFamilyIndices indices;
		bool               raytracingSupport = false;
	};

	class VulkanDevice final
	{
	  public:
		VulkanDevice();
		~VulkanDevice();

		auto init() -> bool;
		auto createPipelineCache() -> void;

		inline const auto getPhysicalDevice() const
		{
			return physicalDevice;
		}
		inline auto getPhysicalDevice()
		{
			return physicalDevice;
		}
		inline auto getGraphicsQueue()
		{
			return graphicsQueue;
		}
		inline auto getPresentQueue()
		{
			return presentQueue;
		}

		inline auto getComputeQueue()
		{
			return computeQueue;
		}

		inline auto getCommandPool()
		{
			return commandPool;
		}
		inline auto getPipelineCache() const
		{
			return pipelineCache;
		}

		static auto get() -> std::shared_ptr<VulkanDevice>
		{
			if (instance == nullptr)
			{
				instance = std::make_shared<VulkanDevice>();
			}
			return instance;
		}

		inline operator auto() const
		{
			return device;
		}

#ifdef USE_VMA_ALLOCATOR
		inline auto getAllocator() const
		{
			return allocator;
		}
#endif

#if defined(MAPLE_PROFILE) && defined(TRACY_ENABLE)
		auto getTracyContext() -> tracy::VkCtx *;
#endif

	  private:
		auto createTracyContext() -> void;

		std::shared_ptr<VulkanPhysicalDevice> physicalDevice;
		std::shared_ptr<VulkanCommandPool>    commandPool;
		static std::shared_ptr<VulkanDevice>  instance;

		VkDevice device = nullptr;
		VkQueue  graphicsQueue;
		VkQueue  presentQueue;
		VkQueue  computeQueue;

		VkPhysicalDeviceFeatures enabledFeatures;
		VkPipelineCache          pipelineCache;

#if defined(MAPLE_PROFILE) && defined(TRACY_ENABLE)
		tracy::VkCtx *tracyContext;
#endif

#ifdef USE_VMA_ALLOCATOR
		VmaAllocator allocator{};
#endif

		bool enableDebugMarkers = false;
	};
};        // namespace maple
