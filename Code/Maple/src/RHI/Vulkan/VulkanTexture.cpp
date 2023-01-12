//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanTexture.h"
#include "IO/Image.h"
#include "IO/ImageLoader.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanSwapChain.h"

#include "Application.h"
#include <cassert>

namespace maple
{
#define DEBUG_IMAGE_ADDRESS(img) void();        //LOGI("{:x},function : {}, line : {}",(uint64_t)img,__FUNCTION__,__LINE__)

	static std::atomic<uint32_t> IdGenerator = 0;

	namespace tools
	{
		inline auto generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t depth, uint32_t mipLevels, uint32_t faces = 1, VkCommandBuffer commandBuffer = nullptr, VkImageLayout initLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) -> void
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(*VulkanDevice::get()->getPhysicalDevice(), imageFormat, &formatProperties);

			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			{
				LOGE("Texture image format does not support linear blitting!");
			}

			bool singleTime = false;

			if (commandBuffer == nullptr)
			{
				commandBuffer = VulkanHelper::beginSingleTimeCommands();
				singleTime    = true;
			}

			VkImageMemoryBarrier barrier{};
			barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image                           = image;
			barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount     = 1;
			barrier.subresourceRange.levelCount     = 1;

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				for (auto face = 0; face < faces; face++)
				{
					barrier.subresourceRange.baseMipLevel   = i - 1;
					barrier.subresourceRange.baseArrayLayer = face;
					barrier.oldLayout                       = initLayout;
					barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

					vkCmdPipelineBarrier(commandBuffer,
					                     VK_PIPELINE_STAGE_TRANSFER_BIT,
					                     VK_PIPELINE_STAGE_TRANSFER_BIT,
					                     0,
					                     0,
					                     nullptr,
					                     0,
					                     nullptr,
					                     1,
					                     &barrier);

					VkImageBlit blit{};
					blit.srcOffsets[0]                 = {0, 0, 0};
					blit.srcOffsets[1]                 = {int32_t(texWidth >> (i - 1)), int32_t(texHeight >> (i - 1)), depth > 1 ? int32_t(depth >> (i - 1)) : 1};
					blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.srcSubresource.mipLevel       = i - 1;
					blit.srcSubresource.baseArrayLayer = face;
					blit.srcSubresource.layerCount     = 1;

					blit.dstOffsets[0] = {0, 0, 0};

					blit.dstOffsets[1] = {
					    std::max(int32_t(texWidth >> i), 1),
					    std::max(int32_t(texHeight >> i), 1),
					    depth > 1 ? int32_t(depth >> i) : 1};

					blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.dstSubresource.mipLevel       = i;
					blit.dstSubresource.baseArrayLayer = face;
					blit.dstSubresource.layerCount     = 1;

					vkCmdBlitImage(commandBuffer,
					               image,
					               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					               image,
					               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					               1,
					               &blit,
					               VK_FILTER_LINEAR);

					barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					vkCmdPipelineBarrier(commandBuffer,
					                     VK_PIPELINE_STAGE_TRANSFER_BIT,
					                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					                     0,
					                     0,
					                     nullptr,
					                     0,
					                     nullptr,
					                     1,
					                     &barrier);
				}
			}

			if (singleTime)
				VulkanHelper::endSingleTimeCommands(commandBuffer);
		}

		inline auto getFormatSize(const TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::NONE:
					return 0;
				case TextureFormat::R8:
					return 1;
				case TextureFormat::RG8:
				case TextureFormat::R16:
					return 2;
				case TextureFormat::RGB8:
				case TextureFormat::RGB:
					return 3;
				case TextureFormat::RGBA8:
				case TextureFormat::RGBA:
				case TextureFormat::RG16F:
				case TextureFormat::R32I:
				case TextureFormat::R32UI:
					return 4;
				case TextureFormat::RGB16:
					return 6;
				case TextureFormat::RGBA16:
					return 8;
				case TextureFormat::RGB32:
					return 12;
				case TextureFormat::RGBA32:
					return 16;
				case TextureFormat::DEPTH:
					return 0;
				case TextureFormat::STENCIL:
					return 0;
				case TextureFormat::DEPTH_STENCIL:
					return 0;
				case TextureFormat::SCREEN:
					return 0;
			}
			MAPLE_ASSERT(false, "Unknown TextureFormat");
		}
	}        // namespace tools

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, const void *data, TextureParameters parameters, TextureLoadOptions loadOptions) :
	    parameters(parameters),
	    loadOptions(loadOptions),
	    data((const uint8_t *) data),
	    width(width),
	    height(height)
	{
		vkFormat = VkConverter::textureFormatToVK(parameters.format, false);

		buildTexture(parameters.format, width, height, false, false, false, loadOptions.generateMipMaps, false, 0);
		update(0, 0, width, height, data);
		id = IdGenerator++;
	}

	VulkanTexture2D::VulkanTexture2D(const std::string &name, const std::string &fileName, TextureParameters parameters, TextureLoadOptions loadOptions) :
	    parameters(parameters),
	    loadOptions(loadOptions),
	    fileName(fileName)
	{
		this->name  = name;
		deleteImage = load();
		if (!deleteImage)
			return;
		id = IdGenerator++;
		createSampler();
	}

	VulkanTexture2D::VulkanTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height) :
	    textureImage(image),
	    textureImageView(imageView),
	    width(width),
	    height(height),
	    vkFormat(format),
	    imageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
	{
		id          = IdGenerator++;
		deleteImage = false;
		updateDescriptor();
	}

	VulkanTexture2D::VulkanTexture2D()
	{
		id             = IdGenerator++;
		deleteImage    = false;
		textureSampler = VulkanHelper::createTextureSampler(
		    VkConverter::textureFilterToVK(parameters.magFilter),
		    VkConverter::textureFilterToVK(parameters.minFilter),
		    0.0f, static_cast<float>(mipLevels), true, VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap));

		updateDescriptor();
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		PROFILE_FUNCTION();
		auto &deletionQueue = VulkanContext::getDeletionQueue();
		for (auto &view : mipImageViews)
		{
			if (view.second)
			{
				auto imageView = view.second;
				deletionQueue.emplace([imageView] { vkDestroyImageView(*VulkanDevice::get(), imageView, nullptr); });
			}
		}

		mipImageViews.clear();

		deleteSampler();
	}

	auto VulkanTexture2D::update(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void *buffer) -> void
	{
		auto stagingBuffer = std::make_unique<VulkanBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, w * h * tools::getFormatSize(parameters.format), buffer);
		auto oldLayout     = imageLayout;
		transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VulkanHelper::copyBufferToImage(stagingBuffer->getVkBuffer(), textureImage, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1, x, y, 0);

		if (loadOptions.generateMipMaps)
		{
			tools::generateMipmaps(textureImage, VkConverter::textureFormatToVK(parameters.format, false), width, height, 1, mipLevels);
		}

		transitionImage(oldLayout);
	}

	auto VulkanTexture2D::load() -> bool
	{
		PROFILE_FUNCTION();
		auto imageSize = tools::getFormatSize(parameters.format) * width * height;

		const uint8_t *pixel = nullptr;

		std::unique_ptr<maple::Image> image;

		if (data != nullptr)
		{
			pixel = data;
			//imageSize         = width * height * 4;
			//parameters.format = TextureFormat::RGBA8;
		}
		else if (fileName != "")
		{
			image             = maple::ImageLoader::loadAsset(fileName, true, loadOptions.flipY);
			width             = image->getWidth();
			height            = image->getHeight();
			imageSize         = image->getImageSize();
			pixel             = reinterpret_cast<const uint8_t *>(image->getData());
			parameters.format = image->getPixelFormat();
			vkFormat          = VkConverter::textureFormatToVK(parameters.format, false);
		}

		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		if (!loadOptions.generateMipMaps)
		{
			mipLevels = 1;
		}

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, mipLevels, VkConverter::textureFormatToVK(parameters.format, false), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0, allocation);
#else
		VulkanHelper::createImage(width, height, mipLevels, VkConverter::textureFormatToVK(parameters.format, false), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0);
#endif
		VulkanHelper::transitionImageLayout(textureImage, VkConverter::textureFormatToVK(parameters.format, false),
		                                    VK_IMAGE_LAYOUT_UNDEFINED,
		                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                                    mipLevels, 1, nullptr, false);

		auto stagingBuffer = std::make_unique<VulkanBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, pixel);
		VulkanHelper::copyBufferToImage(stagingBuffer->getVkBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		if (loadOptions.generateMipMaps && mipLevels > 1)
			tools::generateMipmaps(textureImage, VkConverter::textureFormatToVK(parameters.format, false), width, height, 1, mipLevels);

		transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		DEBUG_IMAGE_ADDRESS(textureImage);
		return true;
	}

	auto VulkanTexture2D::updateDescriptor() -> void
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTexture2D::buildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb, bool depth, bool samplerShadow, bool mipmap, bool image, uint32_t accessFlag) -> void
	{
		PROFILE_FUNCTION();

		deleteSampler();

		this->width  = width;
		this->height = height;
		deleteImage  = true;
		mipLevels    = 1;

		if (loadOptions.generateMipMaps)
		{
			mipLevels = loadOptions.maxMipMaps == -1 ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : loadOptions.maxMipMaps;
		}

		constexpr uint32_t FLAGS = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		vkFormat = VkConverter::textureFormatToVK(internalformat, srgb);

		parameters.format = internalformat;

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, mipLevels, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0, allocation);
#else
		VulkanHelper::createImage(width, height, mipLevels, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0);
#endif

		textureImageView = VulkanHelper::createImageView(textureImage, vkFormat, mipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		textureSampler   = VulkanHelper::createTextureSampler(
            VkConverter::textureFilterToVK(parameters.magFilter),
            VkConverter::textureFilterToVK(parameters.minFilter),
            0.0f, static_cast<float>(mipLevels), true,
            VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
            VkConverter::textureWrapToVK(parameters.wrap),
            VkConverter::textureWrapToVK(parameters.wrap),
            VkConverter::textureWrapToVK(parameters.wrap));

		imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		transitionImage(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		updateDescriptor();

		setName(name);

		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTexture2D::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		if (commandBuffer)
			MAPLE_ASSERT(commandBuffer->isRecording(), "must recording");

		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, VkConverter::textureFormatToVK(parameters.format, false), imageLayout, newLayout, mipLevels, 1, commandBuffer, false);
		}
		imageLayout = newLayout;
		updateDescriptor();
	}

	auto VulkanTexture2D::getMipImageView(uint32_t mip) -> VkImageView
	{
		PROFILE_FUNCTION();
		if (auto iter = mipImageViews.find(mip); iter == mipImageViews.end())
		{
			mipImageViews[mip] = VulkanHelper::createImageView(textureImage, VkConverter::textureFormatToVK(parameters.format, false),
			                                                   mipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, mip);
		}
		return mipImageViews.at(mip);
	}

	auto VulkanTexture2D::memoryBarrier(const CommandBuffer *cmd, uint32_t flags) -> void
	{
		auto layout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (flags & MemoryBarrierFlags::Shader_Image_Access_Barrier)
		{
			layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		//for vulkan, set it as read only after dispatch finished.
		if (flags & MemoryBarrierFlags::Shader_Storage_Barrier)
		{
			layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		if (flags & MemoryBarrierFlags::General)
		{
			layout = VK_IMAGE_LAYOUT_GENERAL;
		}

		if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
		{
			VkImageSubresourceRange subresourceRange;

			subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel   = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount     = 1;
			subresourceRange.levelCount     = 1;

			VulkanHelper::setImageLayout(
			    static_cast<const VulkanCommandBuffer *>(cmd)->getCommandBuffer(),
			    textureImage,
			    imageLayout,
			    layout,
			    subresourceRange);
			imageLayout = layout;
		}
	}

	auto VulkanTexture2D::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
	}

	auto VulkanTexture2D::createSampler() -> void
	{
		PROFILE_FUNCTION();
		textureImageView = VulkanHelper::createImageView(textureImage,
		                                                 VkConverter::textureFormatToVK(parameters.format, false),
		                                                 mipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		auto phyDevice = VulkanDevice::get()->getPhysicalDevice();

		textureSampler = VulkanHelper::createTextureSampler(
		    VkConverter::textureFilterToVK(parameters.magFilter),
		    VkConverter::textureFilterToVK(parameters.minFilter), 0.0f, static_cast<float>(mipLevels), true,
		    phyDevice->getProperties().limits.maxSamplerAnisotropy,
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap));

		updateDescriptor();
	}

	auto VulkanTexture2D::deleteSampler() -> void
	{
		PROFILE_FUNCTION();

		updated = true;

		auto &deletionQueue = VulkanContext::getDeletionQueue();

		if (textureSampler)
		{
			auto sampler = textureSampler;
			deletionQueue.emplace([sampler] { vkDestroySampler(*VulkanDevice::get(), sampler, nullptr); });
		}

		if (textureImageView)
		{
			auto imageView = textureImageView;
			deletionQueue.emplace([imageView] { vkDestroyImageView(*VulkanDevice::get(), imageView, nullptr); });
		}

		if (deleteImage)
		{
			auto image = textureImage;

#ifdef USE_VMA_ALLOCATOR
			auto alloc = allocation;
			deletionQueue.emplace([image, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), image, alloc); });
#else
			deletionQueue.emplace([image] { vkDestroyImage(*VulkanDevice::get(), image, nullptr); });
			if (textureImageMemory)
			{
				auto memory = textureImageMemory;
				deletionQueue.emplace([memory] { vkFreeMemory(*VulkanDevice::get(), memory, nullptr); });
			}
#endif
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VulkanTextureDepth::VulkanTextureDepth(uint32_t width, uint32_t height, bool stencil, const CommandBuffer *commandBuffer) :
	    stencil(stencil),
	    width(width),
	    height(height)
	{
		id = IdGenerator++;

		init(commandBuffer);
	}

	VulkanTextureDepth::~VulkanTextureDepth()
	{
		release();
	}

	auto VulkanTextureDepth::release() -> void
	{
		PROFILE_FUNCTION();
		auto &deletionQueue = VulkanContext::getDeletionQueue();

		imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		updated     = true;
		if (textureSampler)
		{
			auto sampler   = textureSampler;
			auto imageView = textureImageView;
			deletionQueue.emplace([sampler, imageView]() {
				auto device = VulkanDevice::get();
				vkDestroyImageView(*device, imageView, nullptr);
				vkDestroySampler(*device, sampler, nullptr);
			});
		}

#ifdef USE_VMA_ALLOCATOR
		auto image = textureImage;
		auto alloc = allocation;

		deletionQueue.emplace([image, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), image, alloc); });
#else
		auto image       = textureImage;
		auto imageMemory = textureImageMemory;

		deletionQueue.emplace([image, imageMemory]() {
			auto device = VulkanDevice::get();
			vkDestroyImage(*device, image, nullptr);
			vkFreeMemory(*device, imageMemory, nullptr);
		});
#endif
	}

	auto VulkanTextureDepth::resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		this->width  = width;
		this->height = height;
		release();
		init(commandBuffer);
	}

	auto VulkanTextureDepth::updateDescriptor() -> void
	{
		PROFILE_FUNCTION();
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTextureDepth::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
	}

	auto VulkanTextureDepth::init(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		vkFormat = VulkanHelper::getDepthFormat(stencil);
		format   = TextureFormat::DEPTH;

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0, allocation);
#else
		VulkanHelper::createImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 1, 0);
#endif

		textureImageView = VulkanHelper::createImageView(textureImage, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);

		textureSampler = VulkanHelper::createTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, 1.0f, true,
		                                                    VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
		                                                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		                                                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		                                                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		updateDescriptor();

		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTextureDepth::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();

		if (commandBuffer)
			MAPLE_ASSERT(commandBuffer->isRecording(), "must recording");

		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, imageLayout, newLayout, 1, 1, commandBuffer);
		}
		imageLayout = newLayout;
		updateDescriptor();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VulkanTextureCube::VulkanTextureCube(const std::array<std::string, 6> &files)
	{
		LOGC("{0} did not implement", __FUNCTION__);
		id = IdGenerator++;
	}

	VulkanTextureCube::VulkanTextureCube(const std::vector<std::string> &files, uint32_t mips, const TextureParameters &params, const TextureLoadOptions &loadOptions, const InputFormat &format)
	{
		LOGC("{0} did not implement", __FUNCTION__);
		id = IdGenerator++;
	}

	VulkanTextureCube::VulkanTextureCube(const std::string &filePath) :
	    size(0)
	{
		id = IdGenerator++;

		files[0] = filePath;
		load(1);
		updateDescriptor();
	}

	VulkanTextureCube::VulkanTextureCube(uint32_t size, TextureFormat format, int32_t numMips) :
	    numMips(numMips),
	    width(size),
	    height(size),
	    deleteImg(false),
	    size(size)
	{
		id = IdGenerator++;

		parameters.format = format;
		init();
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		auto &deletionQueue = VulkanContext::getDeletionQueue();

		if (textureSampler)
		{
			auto sampler = textureSampler;
			deletionQueue.emplace([sampler] { vkDestroySampler(*VulkanDevice::get(), sampler, nullptr); });
		}

		if (textureImageView)
		{
			auto imageView = textureImageView;
			deletionQueue.emplace([imageView] { vkDestroyImageView(*VulkanDevice::get(), imageView, nullptr); });
		}

		if (deleteImg)
		{
			auto image = textureImage;
#ifdef USE_VMA_ALLOCATOR
			auto alloc = allocation;
			deletionQueue.emplace([image, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), image, alloc); });
#else
			deletionQueue.emplace([image] { vkDestroyImage(*VulkanDevice::get(), image, nullptr); });
			if (textureImageMemory)
			{
				auto imageMemory = textureImageMemory;
				deletionQueue.emplace([imageMemory] { vkFreeMemory(*VulkanDevice::get(), imageMemory, nullptr); });
			}
#endif
		}
	}

	auto VulkanTextureCube::generateMipmap(const CommandBuffer *commandBuffer) -> void
	{
		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);
		transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkCmd);
		tools::generateMipmaps(textureImage, vkFormat, size, size, 1, numMips, 6, vkCmd->getCommandBuffer());
		transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkCmd);
		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		updateDescriptor();
		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTextureCube::updateDescriptor() -> void
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTextureCube::load(uint32_t mips) -> void
	{
		PROFILE_FUNCTION();
	}

	auto VulkanTextureCube::update(const CommandBuffer *commandBuffer, FrameBuffer *framebuffer, int32_t cubeIndex, int32_t mipMapLevel) -> void
	{
		PROFILE_FUNCTION();
		auto cmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		auto frameBuffer = static_cast<VulkanFrameBuffer *>(framebuffer);

		auto &info = frameBuffer->getFrameBufferInfo();
		//TODO..... avoid using dynamic_cast
		auto vkTexture  = dynamic_cast<VkTexture *>(info.attachments[0].get());
		auto colorImage = vkTexture->getImage();
		auto oldLayout  = vkTexture->getImageLayout();

		assert(colorImage != nullptr);

		//set itself as transfer-dst
		{
			transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);
		}
		//set the attachment as TRANSFER-src
		{
			vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);
		}

		// Copy region for transfer from framebuffer to cube face
		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel       = 0;
		copyRegion.srcSubresource.layerCount     = 1;
		copyRegion.srcOffset                     = {0, 0, 0};

		copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = cubeIndex;
		copyRegion.dstSubresource.mipLevel       = mipMapLevel;
		copyRegion.dstSubresource.layerCount     = 1;
		copyRegion.dstOffset                     = {0, 0, 0};

		auto mipScale = std::pow(0.5, mipMapLevel);

		copyRegion.extent.width  = width * mipScale;
		copyRegion.extent.height = height * mipScale;
		copyRegion.extent.depth  = 1;

		// Put image copy into command buffer
		vkCmdCopyImage(
		    cmd->getCommandBuffer(),
		    colorImage,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    textureImage,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &copyRegion);

		// Transform framebuffer color attachment back
		vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);

		transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd);
	}

	auto VulkanTextureCube::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer /*= nullptr*/) -> void
	{
		PROFILE_FUNCTION();
		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, imageLayout, newLayout, numMips, 6, commandBuffer, false);
		}
		imageLayout = newLayout;
		updateDescriptor();
	}

	auto VulkanTextureCube::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
	}

	auto VulkanTextureCube::init() -> void
	{
		PROFILE_FUNCTION();
		vkFormat = VkConverter::textureFormatToVK(parameters.format);

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, numMips, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, allocation);
#else
		VulkanHelper::createImage(width, height, numMips, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
#endif

		/*
		VkCommandBuffer cmdBuffer = VulkanHelper::beginSingleTimeCommands();

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel            = 0;
		subresourceRange.levelCount              = numMips;
		subresourceRange.layerCount              = 6;

		VulkanHelper::setImageLayout(
			cmdBuffer,
			textureImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			subresourceRange);

		VulkanHelper::endSingleTimeCommands(cmdBuffer);*/

		textureSampler = VulkanHelper::createTextureSampler(
		    VK_FILTER_LINEAR,
		    VK_FILTER_LINEAR,
		    0.0f, static_cast<float>(numMips),
		    true,
		    VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
		    VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

		textureImageView = VulkanHelper::createImageView(textureImage, vkFormat, numMips,
		                                                 VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, 6);

		imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DEBUG_IMAGE_ADDRESS(textureImage);

		updateDescriptor();
	}

	VulkanTextureDepthArray::VulkanTextureDepthArray(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) :
	    width(width),
	    height(height),
	    count(count)
	{
		id = IdGenerator++;

		init(commandBuffer);
	}

	VulkanTextureDepthArray::~VulkanTextureDepthArray()
	{
		release();
	}

	auto VulkanTextureDepthArray::resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) -> void
	{
		this->width  = width;
		this->height = height;
		this->count  = count;

		release();
		init(commandBuffer);
	}

	auto VulkanTextureDepthArray::getHandleArray(uint32_t index) -> void *
	{
		descriptor.imageView = getImageView(index);
		return (void *) &descriptor;
	}

	auto VulkanTextureDepthArray::updateDescriptor() -> void
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTextureDepthArray::init(const CommandBuffer *commandBuffer) -> void
	{
		auto depthFormat = VulkanHelper::getDepthFormat();

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, count, 1, allocation);
#else
		VulkanHelper::createImage(width, height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, count, 1);
#endif
		textureImageView = VulkanHelper::createImageView(textureImage, depthFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, count);
		for (uint32_t i = 0; i < count; i++)
		{
			imageViews.emplace_back(VulkanHelper::createImageView(textureImage, depthFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, 1, i));
		}
		format = TextureFormat::DEPTH;

		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		VulkanHelper::transitionImageLayout(textureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
		                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, count, vkCmd, true);

		imageLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		textureSampler = VulkanHelper::createTextureSampler();
		updateDescriptor();
		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTextureDepthArray::release() -> void
	{
		auto &queue = VulkanContext::getDeletionQueue();

		auto textureImageView   = this->textureImageView;
		auto textureImage       = this->textureImage;
		auto textureImageMemory = this->textureImageMemory;
		auto textureSampler     = this->textureSampler;
		auto imageViews         = this->imageViews;
		auto size               = count;
		updated                 = true;
		queue.emplace([textureImageView, textureSampler, imageViews, size]() {
			vkDestroyImageView(*VulkanDevice::get(), textureImageView, nullptr);

			if (textureSampler)
				vkDestroySampler(*VulkanDevice::get(), textureSampler, nullptr);

			for (uint32_t i = 0; i < size; i++)
			{
				vkDestroyImageView(*VulkanDevice::get(), imageViews[i], nullptr);
			}
		});

#ifdef USE_VMA_ALLOCATOR
		auto alloc = allocation;
		queue.emplace([textureImage, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), textureImage, alloc); });
#else
		queue.emplace([textureImage, textureImageMemory]() {
			vkDestroyImage(*VulkanDevice::get(), textureImage, nullptr);
			vkFreeMemory(*VulkanDevice::get(), textureImageMemory, nullptr);
		});
#endif
	}

	auto VulkanTextureDepthArray::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, VulkanHelper::getDepthFormat(), imageLayout, newLayout, 1, count, commandBuffer);
		}
		imageLayout = newLayout;
		updateDescriptor();
	}

	auto VulkanTextureDepthArray::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
		for (int32_t i = 0; i < imageViews.size(); i++)
		{
			VulkanHelper::setObjectName("ImageView:" + name + "_" + std::to_string(i), (uint64_t) imageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW);
		}
	}

	VulkanTexture3D::VulkanTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters, TextureLoadOptions loadOptions, const void *data) :
	    width(width),
	    height(height),
	    depth(depth),
	    parameters(parameters),
	    loadOptions(loadOptions)
	{
		id = IdGenerator++;

		init(width, height, depth, data);
	}

	VulkanTexture3D::~VulkanTexture3D()
	{
		deleteSampler();
	}

	auto VulkanTexture3D::init(uint32_t width, uint32_t height, uint32_t depth, const void *data) -> void
	{
		buildTexture3D(parameters.format, width, height, depth);

		if (data != nullptr)
		{
			auto stagingBuffer = std::make_unique<VulkanBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, width * height * depth * tools::getFormatSize(parameters.format), data);
			auto oldLayout     = imageLayout;
			transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VulkanHelper::copyBufferToImage(stagingBuffer->getVkBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), depth, 0, 0, 0);
			if (loadOptions.generateMipMaps)
			{
				tools::generateMipmaps(textureImage, VkConverter::textureFormatToVK(parameters.format, false), width, height, depth, mipLevels);
			}
			transitionImage(oldLayout);
		}
	}

	auto VulkanTexture3D::bind(uint32_t slot) const -> void
	{
	}

	auto VulkanTexture3D::unbind(uint32_t slot) const -> void
	{
	}

	auto VulkanTexture3D::generateMipmaps(const CommandBuffer *cmd) -> void
	{
		if (loadOptions.generateMipMaps && mipLevels > 1)
		{
			tools::generateMipmaps(textureImage, VkConverter::textureFormatToVK(parameters.format, false), width, height, depth, mipLevels);
		}
	}

	auto VulkanTexture3D::bindImageTexture(uint32_t unit, bool read, bool write, uint32_t level, uint32_t layer, TextureFormat format) -> void
	{
	}

	auto VulkanTexture3D::buildTexture3D(TextureFormat format, uint32_t width, uint32_t height, uint32_t depth) -> void
	{
		PROFILE_FUNCTION();
		this->width  = width;
		this->height = height;
		this->depth  = depth;

		mipLevels = 1;
		if (loadOptions.generateMipMaps)
			mipLevels = loadOptions.maxMipMaps == -1 ? static_cast<uint32_t>(std::floor(std::log2(std::max(std::max(width, height), depth)))) + 1 : loadOptions.maxMipMaps;

		deleteSampler();

		vkFormat = VkConverter::textureFormatToVK(format, false);

		parameters.format = format;

		VkImageCreateFlags createFlags = 0;
		/*VkImageFormatListCreateInfo imageFormatListInfo = {};
		std::array<VkFormat, 3> formats = {
			VK_FORMAT_R32_UINT,
			VK_FORMAT_R32_SINT,
			VK_FORMAT_R8G8B8A8_UNORM,
		};*/

		if (loadOptions.mutableFormat)
		{
			createFlags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
			/*imageFormatListInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
			imageFormatListInfo.pViewFormats = formats.data();
			imageFormatListInfo.viewFormatCount = formats.size();*/

			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(*VulkanDevice::get()->getPhysicalDevice(), vkFormat, &formatProperties);
			/**
* Vulkan specification guarantees that atomic operations 
* must be supported for storage images (VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT) with only R32_UINT and R32_SINT formats.
* which means R32 can be converted to R8, but R8 can not be converted R32.
*/
			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT))
			{
				MAPLE_ASSERT(false, "Provided format is not supported for atomic operations on storage images. the supported should be R32_UINT or R32_SINT");
			}
		}

		VulkanHelper::createImage(width, height, mipLevels, vkFormat,
		                          VK_IMAGE_TYPE_3D, VK_IMAGE_TILING_OPTIMAL,
		                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
#ifdef USE_VMA_ALLOCATOR
		                          textureImage, textureImageMemory, 1, createFlags, allocation, depth, VK_IMAGE_LAYOUT_UNDEFINED, nullptr);
#else
		                          textureImage, textureImageMemory, 1, createFlags, depth, VK_IMAGE_LAYOUT_UNDEFINED, nullptr);
#endif

		textureImageView = VulkanHelper::createImageView(textureImage, vkFormat, mipLevels, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		if (loadOptions.mutableFormat)
		{
			linearTextureSampler = VulkanHelper::createTextureSampler(
			    VK_FILTER_LINEAR,
			    VK_FILTER_LINEAR,
			    0.0f, static_cast<float>(mipLevels), true,
			    VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
			    VkConverter::textureWrapToVK(parameters.wrap),
			    VkConverter::textureWrapToVK(parameters.wrap),
			    VkConverter::textureWrapToVK(parameters.wrap));
		}

		textureSampler = VulkanHelper::createTextureSampler(
		    VkConverter::textureFilterToVK(parameters.magFilter),
		    VkConverter::textureFilterToVK(parameters.minFilter),
		    0.0f, static_cast<float>(mipLevels), true,
		    VulkanDevice::get()->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap),
		    VkConverter::textureWrapToVK(parameters.wrap));

		imageLayouts.resize(mipLevels, VK_IMAGE_LAYOUT_UNDEFINED);

		transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		if (loadOptions.generateMipMaps && mipLevels > 1)
		{
			tools::generateMipmaps(textureImage, vkFormat, width, height, depth, mipLevels);

			for (auto i = 0; i < mipLevels; i++)
			{
				mipmapVies.emplace_back(
				    VulkanHelper::createImageView(textureImage, vkFormat, 1, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, i));
			}
		}

		updateDescriptor();
		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTexture3D::update(const CommandBuffer *cmd, const void *buffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		if (buffer != nullptr)
		{
			auto stagingBuffer = std::make_unique<VulkanBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, width * height * depth * tools::getFormatSize(parameters.format), buffer);
			auto oldLayout     = imageLayout;
			transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<const VulkanCommandBuffer *>(cmd));
			VulkanHelper::copyBufferToImage(stagingBuffer->getVkBuffer(),
			                                textureImage,
			                                static_cast<uint32_t>(width),
			                                static_cast<uint32_t>(height),
			                                static_cast<uint32_t>(depth), x, y, z,
			                                static_cast<const VulkanCommandBuffer *>(cmd));
			transitionImage(oldLayout, static_cast<const VulkanCommandBuffer *>(cmd));
		}
	}

	auto VulkanTexture3D::updateMipmap(const CommandBuffer *cmd, uint32_t mipLevel, const void *buffer) -> void
	{
		if (buffer != nullptr)
		{
			auto vkCmd         = static_cast<const VulkanCommandBuffer *>(cmd);
			auto w             = width >> mipLevel;
			auto h             = height >> mipLevel;
			auto d             = depth >> mipLevel;
			auto stagingBuffer = std::make_unique<VulkanBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, w * h * d * tools::getFormatSize(parameters.format), buffer);
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, imageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1, vkCmd, false, 0, mipLevel);
			VkBufferImageCopy bufferCopyRegion{};
			bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel       = mipLevel;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount     = 1;
			bufferCopyRegion.imageExtent.width               = static_cast<uint32_t>(w);
			bufferCopyRegion.imageExtent.height              = static_cast<uint32_t>(h);
			bufferCopyRegion.imageExtent.depth               = static_cast<uint32_t>(d);
			vkCmdCopyBufferToImage(vkCmd->getCommandBuffer(), stagingBuffer->getVkBuffer(), textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, 1, 1, vkCmd, false, 0, mipLevel);
		}
	}

	auto VulkanTexture3D::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
	}

	auto VulkanTexture3D::transitionImage2(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer /*= nullptr*/, int32_t mipLevel /*= 0*/) -> void
	{
		PROFILE_FUNCTION();
		if (commandBuffer)
			MAPLE_ASSERT(commandBuffer->isRecording(), "must recording");

		auto oldLayout = mipLevels > 1 && mipLevel >= 0 ? imageLayouts[mipLevel] : imageLayout;

		if (newLayout != oldLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, oldLayout, newLayout, mipLevel >= 0 ? 1 : mipLevels, 1, commandBuffer, false, 0, std::max(mipLevel, 0));
		}
		if (mipLevels > 1 && mipLevel >= 0)
			imageLayouts[mipLevel] = newLayout;
		else
			imageLayout = newLayout;
	}

	auto VulkanTexture3D::clear(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.baseMipLevel            = 0;
		subresourceRange.layerCount              = 1;
		subresourceRange.levelCount              = mipLevels;
		subresourceRange.baseArrayLayer          = 0;
		subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
		VkClearColorValue clearColourValue       = VkClearColorValue({{0, 0, 0, 0}});
		vkCmdClearColorImage(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), textureImage, imageLayout, &clearColourValue, 1, &subresourceRange);
	}

	auto VulkanTexture3D::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer /*= nullptr*/) -> void
	{
		PROFILE_FUNCTION();
		if (commandBuffer)
			MAPLE_ASSERT(commandBuffer->isRecording(), "must recording");

		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, vkFormat, imageLayout, newLayout, mipLevels, 1, commandBuffer, false);
		}
		imageLayout = newLayout;
	}

	auto VulkanTexture3D::deleteSampler() -> void
	{
		PROFILE_FUNCTION();

		updated = true;

		auto &deletionQueue = VulkanContext::getDeletionQueue();

		if (textureSampler)
		{
			auto sampler = textureSampler;
			deletionQueue.emplace([sampler] { vkDestroySampler(*VulkanDevice::get(), sampler, nullptr); });
		}

		for (auto textureImageView : mipmapVies)
		{
			auto imageView = textureImageView;
			deletionQueue.emplace([imageView] { vkDestroyImageView(*VulkanDevice::get(), imageView, nullptr); });
		}

		auto image = textureImage;

#ifdef USE_VMA_ALLOCATOR
		auto alloc = allocation;
		deletionQueue.emplace([image, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), image, alloc); });
#else
		deletionQueue.emplace([image] { vkDestroyImage(*VulkanDevice::get(), image, nullptr); });
		if (textureImageMemory)
		{
			auto memory = textureImageMemory;
			deletionQueue.emplace([memory] { vkFreeMemory(*VulkanDevice::get(), memory, nullptr); });
		}
#endif
	}

	auto VulkanTexture3D::updateDescriptor() -> void
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTexture3D::getDescriptorInfo(int32_t mipLvl, TextureFormat format) -> void *
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = mipLevels > 1 && mipLvl >= 0 ? mipmapVies[mipLvl] : textureImageView;
		descriptor.imageLayout = mipLevels > 1 && mipLvl >= 0 ? imageLayouts[mipLvl] : imageLayout;
		//sampler.  convert is as RGBA8 imageView

		if (format != parameters.format)
		{
			if (parameters.format == TextureFormat::R32I || parameters.format == TextureFormat::R32UI)
			{
				if (format == TextureFormat::NONE)
					format = TextureFormat::RGBA8;

				size_t id = 0;
				hash::hashCode(id, mipLvl, format);

				descriptor.sampler = linearTextureSampler;

				auto iter = formatToLayout.find(id);
				if (iter == formatToLayout.end())
				{
					auto imageView = mipLvl >= 0 ? VulkanHelper::createImageView(textureImage,
					                                                             VkConverter::textureFormatToVK(format, false),
					                                                             1, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, mipLvl)

					                               :

                                                   VulkanHelper::createImageView(textureImage,
					                                                             VkConverter::textureFormatToVK(format, false),
					                                                             mipLevels, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, 1);

					iter = formatToLayout.emplace(id, imageView).first;
				}
				descriptor.imageView = iter->second;
			}
		}
		return (void *) &descriptor;
	}

	VulkanTexture2DArray::VulkanTexture2DArray(uint32_t width, uint32_t height, uint32_t count, TextureFormat format, TextureParameters parameters, const CommandBuffer *commandBuffer) :
	    width(width),
	    height(height),
	    count(count),
	    format(format),
	    parameters(parameters)
	{
		id = IdGenerator++;

		init(commandBuffer);
	}

	VulkanTexture2DArray::~VulkanTexture2DArray()
	{
		release();
	}

	auto VulkanTexture2DArray::resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) -> void
	{
		this->width  = width;
		this->height = height;
		this->count  = count;
		release();
		init(commandBuffer);
	}

	auto VulkanTexture2DArray::getHandleArray(uint32_t index) -> void *
	{
		descriptor.imageView = getImageView(index);
		return (void *) &descriptor;
	}

	auto VulkanTexture2DArray::updateDescriptor() -> void
	{
		descriptor.sampler     = textureSampler;
		descriptor.imageView   = textureImageView;
		descriptor.imageLayout = imageLayout;
	}

	auto VulkanTexture2DArray::transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		if (newLayout != imageLayout)
		{
			VulkanHelper::transitionImageLayout(textureImage, VkConverter::textureFormatToVK(format), imageLayout, newLayout, 1, count, commandBuffer);
		}
		imageLayout = newLayout;
		updateDescriptor();
	}

	auto VulkanTexture2DArray::init(const CommandBuffer *commandBuffer) -> void
	{
		auto vkFormat = VkConverter::textureFormatToVK(format);

#ifdef USE_VMA_ALLOCATOR
		VulkanHelper::createImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, count, 1, allocation);
#else
		VulkanHelper::createImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, count, 1);
#endif
		textureImageView = VulkanHelper::createImageView(textureImage, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, count);
		for (uint32_t i = 0; i < count; i++)
		{
			imageViews.emplace_back(VulkanHelper::createImageView(textureImage, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, 1, i));
		}

		auto vkCmd = static_cast<const VulkanCommandBuffer *>(commandBuffer);

		VulkanHelper::transitionImageLayout(textureImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1, count, vkCmd, false);

		imageLayout    = VK_IMAGE_LAYOUT_GENERAL;
		textureSampler = VulkanHelper::createTextureSampler();
		updateDescriptor();
		DEBUG_IMAGE_ADDRESS(textureImage);
	}

	auto VulkanTexture2DArray::release() -> void
	{
		auto &queue = VulkanContext::getDeletionQueue();

		auto textureImageView   = this->textureImageView;
		auto textureImage       = this->textureImage;
		auto textureImageMemory = this->textureImageMemory;
		auto textureSampler     = this->textureSampler;
		auto imageViews         = this->imageViews;
		auto size               = count;
		updated                 = true;
		queue.emplace([textureImageView, textureSampler, imageViews, size]() {
			vkDestroyImageView(*VulkanDevice::get(), textureImageView, nullptr);

			if (textureSampler)
				vkDestroySampler(*VulkanDevice::get(), textureSampler, nullptr);

			for (uint32_t i = 0; i < size; i++)
			{
				vkDestroyImageView(*VulkanDevice::get(), imageViews[i], nullptr);
			}
		});

#ifdef USE_VMA_ALLOCATOR
		auto alloc = allocation;
		queue.emplace([textureImage, alloc] { vmaDestroyImage(VulkanDevice::get()->getAllocator(), textureImage, alloc); });
#else
		queue.emplace([textureImage, textureImageMemory]() {
			vkDestroyImage(*VulkanDevice::get(), textureImage, nullptr);
			vkFreeMemory(*VulkanDevice::get(), textureImageMemory, nullptr);
		});
#endif
	}

	auto VulkanTexture2DArray::setName(const std::string &name) -> void
	{
		Texture::setName(name);
		VulkanHelper::setObjectName("Image:" + name, (uint64_t) textureImage, VK_OBJECT_TYPE_IMAGE);
		VulkanHelper::setObjectName("ImageView:" + name, (uint64_t) textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
		for (int32_t i = 0; i < imageViews.size(); i++)
		{
			VulkanHelper::setObjectName("ImageView:" + name + "_" + std::to_string(i), (uint64_t) imageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW);
		}
	}

};        // namespace maple
