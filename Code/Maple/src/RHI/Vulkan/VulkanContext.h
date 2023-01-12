//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/GraphicsContext.h"
#include <deque>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace maple
{
	class UniformBuffer;
	class VulkanFence;

	class MAPLE_EXPORT VulkanContext : public GraphicsContext
	{
	  public:
		VulkanContext();
		~VulkanContext();

		NO_COPYABLE(VulkanContext);
		auto init() -> void override;
		auto present() -> void override;
		auto getMinUniformBufferOffsetAlignment() const -> size_t override;
		auto alignedDynamicUboSize(size_t size) const -> size_t override;

		auto immediateSubmit(const std::function<void(CommandBuffer *)> &execute) -> void override;

		auto isRaytracingSupported() -> bool override;

		auto waitIdle() const -> void override;
		auto onImGui() -> void override;

		inline auto getGPUMemoryUsed() -> float override
		{
			return 0;
		}

		inline auto getTotalGPUMemory() -> float override
		{
			return 0;
		}

		inline const auto getVkInstance() const
		{
			return vkInstance;
		}
		inline auto getVkInstance()
		{
			return vkInstance;
		}

		struct CommandQueue
		{
			CommandQueue()                     = default;
			CommandQueue(const CommandQueue &) = delete;
			auto operator=(const CommandQueue &) -> CommandQueue & = delete;

			std::deque<std::function<void()>> deletors;

			template <typename F>
			inline auto emplace(F &&function)
			{
				deletors.emplace_back(function);
			}

			inline auto flush()
			{
				for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
				{
					(*it)();
				}
				deletors.clear();
			}
		};

		static auto get() -> std::shared_ptr<VulkanContext>;

		static auto getDeletionQueue() -> CommandQueue &;
		static auto getDeletionQueue(uint32_t index) -> CommandQueue &;

	  private:
		auto setupDebug() -> void;

		VkInstance vkInstance = nullptr;

		//bind to triple buffer
		CommandQueue deletionQueue[3];

		std::vector<const char *>          instanceLayerNames;
		std::vector<const char *>          instanceExtensionNames;
		std::vector<VkLayerProperties>     instanceLayers;
		std::vector<VkExtensionProperties> instanceExtensions;

		auto createInstance() -> void;

		VkDebugUtilsMessengerEXT debugMessenger;

		std::unordered_map<size_t, std::shared_ptr<UniformBuffer>> uniformBuffer;

		std::unique_ptr<VulkanFence> updateFence;
	};

};        // namespace maple
