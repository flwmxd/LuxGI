//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "RHI/SwapChain.h"
#include "RHI/Texture.h"
#include "VulkanHelper.h"
#include <memory>

namespace maple
{
	constexpr uint32_t MAX_SWAPCHAIN_BUFFERS = 3;

	struct FrameData
	{
		std::shared_ptr<VulkanCommandPool>   commandPool;
		std::shared_ptr<VulkanCommandBuffer> commandBuffer;
		VkSemaphore                          presentSemaphore = VK_NULL_HANDLE;
	};

	struct ComputeData
	{
		//VkQueue queue;										// Separate queue for compute commands (queue family may differ from the one used for graphics)
		std::shared_ptr<VulkanCommandPool>   commandPool;                       // Use a separate command pool (queue family may differ from the one used for graphics)
		std::shared_ptr<VulkanCommandBuffer> commandBuffer;                     // Command buffer storing the dispatch commands and barriers
		VkSemaphore                          semaphore = VK_NULL_HANDLE;        // Execution dependency between compute & graphic submission
	};

	class VulkanSwapChain final : public SwapChain
	{
	  public:
		VulkanSwapChain(uint32_t width, uint32_t height);
		NO_COPYABLE(VulkanSwapChain);
		virtual ~VulkanSwapChain();

		auto init(bool vsync, NativeWindow *window) -> bool override;
		auto init(bool vsync) -> bool override;

		auto getCurrentImage() -> std::shared_ptr<Texture> override
		{
			return swapChainBuffers[acquireImageIndex];
		}
		auto getImage(uint32_t index) -> std::shared_ptr<Texture> override
		{
			return swapChainBuffers[index];
		}
		auto getCurrentBufferIndex() const -> uint32_t override
		{
			return currentBuffer;
		}
		auto getCurrentImageIndex() const -> uint32_t override
		{
			return acquireImageIndex;
		}
		auto getSwapChainBufferCount() const -> size_t override
		{
			return swapChainBufferCount;
		}

		inline auto getScreenFormat() const
		{
			return colorFormat;
		}
		inline auto getFormat()
		{
			return colorFormat;
		}

		inline auto getSurface() const
		{
			return surface;
		}

		auto queueSubmit() -> void;
		auto begin() -> void;
		auto end() -> void;

		auto getCurrentCommandBuffer() -> CommandBuffer * override;

		auto onResize(uint32_t width, uint32_t height, bool forceResize = false, NativeWindow *windowHandle = nullptr) -> void;

		auto acquireNextImage() -> void;

		auto present() -> void;

		auto getFrameData() -> FrameData &;

		autoUnpack(swapChain);

		auto getComputeCmdBuffer() -> CommandBuffer * override;

	  private:
		auto createFrameData() -> void;
		auto createComputeData() -> void;

		FrameData frames[MAX_SWAPCHAIN_BUFFERS];

		auto findImageFormatAndColorSpace() -> void;

		std::vector<std::shared_ptr<Texture2D>> swapChainBuffers;

		uint32_t currentBuffer        = 0;
		uint32_t acquireImageIndex    = 0;
		uint32_t queueNodeIndex       = -1;
		uint32_t width                = 0;
		uint32_t height               = 0;
		uint32_t swapChainBufferCount = 0;
		bool     vsync                = false;

		VkSwapchainKHR  swapChain    = nullptr;
		VkSwapchainKHR  oldSwapChain = nullptr;
		VkSurfaceKHR    surface      = nullptr;
		VkFormat        colorFormat;
		VkColorSpaceKHR colorSpace;
		ComputeData     computeData;

		// Execution dependency between compute & graphic submission
		VkSemaphore graphicsSemaphore = nullptr;
	};
};        // namespace maple
