#include "VKImGuiRenderer.h"
#include "Application.h"
#include "RHI/Vulkan/Vk.h"
#include "RHI/Vulkan/VulkanCommandBuffer.h"
#include "RHI/Vulkan/VulkanContext.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RHI/Vulkan/VulkanFrameBuffer.h"
#include "RHI/Vulkan/VulkanRenderPass.h"
#include "RHI/Vulkan/VulkanSwapChain.h"
#include "RHI/Vulkan/VulkanTexture.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Engine/Profiler.h"

namespace maple
{
	static ImGui_ImplVulkanH_Window g_WindowData;
	static VkAllocationCallbacks *  g_Allocator        = nullptr;
	static VkDescriptorPool         g_DescriptorPool   = VK_NULL_HANDLE;
	static bool                     g_SwapChainRebuild = false;

	VKImGuiRenderer::VKImGuiRenderer(uint32_t width, uint32_t height, bool clearScreen) :
	    clearScreen(clearScreen),
	    width(width),
	    height(height)
	{
		PROFILE_FUNCTION();
	}

	VKImGuiRenderer::~VKImGuiRenderer()
	{
		PROFILE_FUNCTION();

		auto &deletionQueue = VulkanContext::getDeletionQueue();

		for (int i = 0; i < VulkanContext::get()->getSwapChain()->getSwapChainBufferCount(); i++)
		{
			ImGui_ImplVulkanH_Frame *fd          = &g_WindowData.Frames[i];
			auto                     fence       = fd->Fence;
			auto                     alloc       = g_Allocator;
			auto                     commandPool = fd->CommandPool;

			deletionQueue.emplace([fence, commandPool, alloc] {
				vkDestroyFence(*VulkanDevice::get(), fence, alloc);
				vkDestroyCommandPool(*VulkanDevice::get(), commandPool, alloc);
			});
		}
		auto descriptorPool = g_DescriptorPool;

		deletionQueue.emplace([descriptorPool] {
			vkDestroyDescriptorPool(*VulkanDevice::get(), descriptorPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		});
	}

	auto VKImGuiRenderer::init() -> void
	{
		PROFILE_FUNCTION();
		ImGui_ImplGlfw_InitForVulkan((GLFWwindow *) Application::getWindow()->getNativeInterface(), true);

		ImGui_ImplVulkanH_Window *wd = &g_WindowData;

		auto vkSwapChain = std::static_pointer_cast<VulkanSwapChain>(VulkanContext::get()->getSwapChain());

		VkSurfaceKHR surface = vkSwapChain->getSurface();
		setupVulkanWindowData(wd, surface, width, height);

		// Setup Vulkan binding
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance                  = VulkanContext::get()->getVkInstance();
		init_info.PhysicalDevice            = *VulkanDevice::get()->getPhysicalDevice();
		init_info.Device                    = *VulkanDevice::get();
		init_info.QueueFamily               = VulkanDevice::get()->getPhysicalDevice()->getQueueFamilyIndices().graphicsFamily.value();
		init_info.Queue                     = VulkanDevice::get()->getGraphicsQueue();
		init_info.PipelineCache             = VulkanDevice::get()->getPipelineCache();
		init_info.DescriptorPool            = g_DescriptorPool;
		init_info.Allocator                 = g_Allocator;
		init_info.CheckVkResultFn           = [](VkResult err) {
            VK_CHECK_RESULT(err);
		};
		init_info.MinImageCount = 2;
		init_info.ImageCount    = (uint32_t) vkSwapChain->getSwapChainBufferCount();
		ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
		// Upload Fonts
		{
			rebuildFontTexture();
		}
	}

	auto VKImGuiRenderer::rebuildFontTexture() -> void
	{
		PROFILE_FUNCTION();
		ImGuiIO &      io = ImGui::GetIO();
		unsigned char *pixels;
		int            width, height;

		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		fontTexture = Texture2D::create(width, height, pixels);
		fontTexture->setName("FontsTextures");
		io.Fonts->TexID = (ImTextureID) fontTexture->getHandle();
	}

	auto VKImGuiRenderer::newFrame(const Timestep &dt) -> void
	{
		PROFILE_FUNCTION();
		ImGuiIO &io  = ImGui::GetIO();
		io.DeltaTime = dt.getMilliseconds();
	
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	auto VKImGuiRenderer::render(CommandBuffer *commandBuffer) -> void
	{
		//GPUProfile("ImGui Pass");
		PROFILE_FUNCTION();
		g_WindowData.FrameIndex = VulkanContext::get()->getSwapChain()->getCurrentImageIndex();
		auto  vkCommnadBuffer    = (VulkanCommandBuffer *) commandBuffer;
		auto &descriptorImageMap = ImGui_ImplVulkan_GetDescriptorImageMap();

		auto drawData = ImGui::GetDrawData();
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList *cmdList = drawData->CmdLists[n];
			for (int32_t cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
			{
				const ImDrawCmd *pcmd = &cmdList->CmdBuffer[cmdIdx];

				if ((Texture *) pcmd->TextureId)
				{
					if (((Texture *) pcmd->TextureId)->getType() == TextureType::Color)
					{
						auto texture = (VulkanTexture2D *) pcmd->TextureId;
						texture->transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkCommnadBuffer);
						descriptorImageMap[pcmd->TextureId] = texture->getDescriptor();
					}
					else if (((Texture *) pcmd->TextureId)->getType() == TextureType::Depth)
					{
						auto texture = (VulkanTextureDepth *) pcmd->TextureId;
						texture->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, vkCommnadBuffer);
						descriptorImageMap[pcmd->TextureId] = texture->getDescriptor();
					}
					else if (((Texture *) pcmd->TextureId)->getType() == TextureType::DepthArray)
					{
						auto texture = (VulkanTextureDepthArray *) pcmd->TextureId;
						texture->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, vkCommnadBuffer);
						descriptorImageMap[pcmd->TextureId] = texture->getDescriptor();
					}
					//	auto text = ((Texture*)pcmd->TextureId);
					//	MAPLE_ASSERT(descriptorImageMap[pcmd->TextureId] != nullptr, text->getName());
				}
			}
		}

		ImGui_ImplVulkan_CreateDescriptorSets(ImGui::GetDrawData(), g_WindowData.FrameIndex);
		renderPass->beginRenderPass(commandBuffer, glm::vec4(0.1f, 0.1f, 0.1f, 1.f), frameBuffers[g_WindowData.FrameIndex].get(), SubPassContents::Inline, g_WindowData.Width, g_WindowData.Height, -1, 0);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), nullptr, g_WindowData.FrameIndex);
		renderPass->endRenderPass(commandBuffer);

		ImGui_ImplVulkanH_Window *wd = &g_WindowData;
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	auto VKImGuiRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
		auto  swapChain = std::static_pointer_cast<VulkanSwapChain>(VulkanContext::get()->getSwapChain());
		auto *wd        = &g_WindowData;
		wd->Swapchain   = *swapChain;
		for (uint32_t i = 0; i < wd->ImageCount; i++)
		{
			auto scBuffer                = (VulkanTexture2D *) swapChain->getImage(i).get();
			wd->Frames[i].Backbuffer     = scBuffer->getImage();
			wd->Frames[i].BackbufferView = scBuffer->getImageView();
		}
		wd->Width  = width;
		wd->Height = height;
		frameBuffers.clear();
		frameBuffers.resize(swapChain->getSwapChainBufferCount());

		RenderPassInfo renderPassDesc;
		renderPassDesc.clear       = clearScreen;
		renderPassDesc.attachments = {swapChain->getImage(0)};

		renderPass     = RenderPass::create(renderPassDesc);
		wd->RenderPass = *std::static_pointer_cast<VulkanRenderPass>(renderPass);

		// Create Framebuffer
		FrameBufferInfo bufferInfo{};
		bufferInfo.width      = wd->Width;
		bufferInfo.height     = wd->Height;
		bufferInfo.renderPass = renderPass;
		bufferInfo.screenFBO  = true;
		bufferInfo.attachments.resize(1);

		for (uint32_t i = 0; i < swapChain->getSwapChainBufferCount(); i++)
		{
			bufferInfo.attachments[0] = swapChain->getImage(i);
			frameBuffers[i]           = FrameBuffer::create(bufferInfo);
			wd->Frames[i].Framebuffer = *std::static_pointer_cast<VulkanFrameBuffer>(frameBuffers[i]);
		}
	}

	auto VKImGuiRenderer::setupVulkanWindowData(ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int32_t width, int32_t height) -> void
	{
		PROFILE_FUNCTION();
		// Create Descriptor Pool
		{
			VkDescriptorPoolSize poolSizes[] = {
			    {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
			    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
			    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
			    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
			    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
			    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
			    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
			    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
			    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
			    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
			    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.maxSets                    = 1000 * IM_ARRAYSIZE(poolSizes);
			poolInfo.poolSizeCount              = (uint32_t) IM_ARRAYSIZE(poolSizes);
			poolInfo.pPoolSizes                 = poolSizes;
			VK_CHECK_RESULT(vkCreateDescriptorPool(*VulkanDevice::get(), &poolInfo, g_Allocator, &g_DescriptorPool));
		}

		wd->Surface     = surface;
		wd->ClearEnable = clearScreen;

		auto swapChain = std::static_pointer_cast<VulkanSwapChain>(VulkanContext::get()->getSwapChain());
		wd->Swapchain  = *swapChain;
		wd->Width      = width;
		wd->Height     = height;

		wd->ImageCount = static_cast<uint32_t>(swapChain->getSwapChainBufferCount());

		RenderPassInfo renderPassDesc;
		renderPassDesc.clear       = clearScreen;
		renderPassDesc.attachments = {swapChain->getImage(0)};

		renderPass     = RenderPass::create(renderPassDesc);
		wd->RenderPass = *std::static_pointer_cast<VulkanRenderPass>(renderPass);

		wd->Frames = (ImGui_ImplVulkanH_Frame *) IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
		memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);

		// Create The Image Views
		{
			for (uint32_t i = 0; i < wd->ImageCount; i++)
			{
				auto scBuffer = std::static_pointer_cast<VulkanTexture2D>(swapChain->getImage(i));

				wd->Frames[i].Backbuffer     = scBuffer->getImage();
				wd->Frames[i].BackbufferView = scBuffer->getImageView();
			}
		}

		FrameBufferInfo frameBufferDesc{};
		frameBufferDesc.width      = wd->Width;
		frameBufferDesc.height     = wd->Height;
		frameBufferDesc.renderPass = renderPass;
		frameBufferDesc.screenFBO  = true;
		std::vector<std::shared_ptr<Texture>> attachments;
		attachments.resize(1);

		frameBuffers.clear();
		frameBuffers.resize(swapChain->getSwapChainBufferCount());

		for (uint32_t i = 0; i < swapChain->getSwapChainBufferCount(); i++)
		{
			attachments[0]              = swapChain->getImage(i);
			frameBufferDesc.attachments = attachments;
			frameBuffers[i]             = FrameBuffer::create(frameBufferDesc);
			wd->Frames[i].Framebuffer   = *std::static_pointer_cast<VulkanFrameBuffer>(frameBuffers[i]);
		}
	}

}        // namespace maple
