//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RHI/Texture.h"
#include "VulkanHelper.h"

namespace maple
{
	class VkTexture
	{
	  public:
		virtual auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr) -> void = 0;
		virtual auto getImageLayout() const -> VkImageLayout                                                              = 0;
		virtual auto setImageLayout(const VkImageLayout &layout) -> void                                                  = 0;
		virtual auto getImage() const -> VkImage                                                                          = 0;
	};

	class VulkanTexture2D : public Texture2D, public VkTexture
	{
	  public:
		VulkanTexture2D(uint32_t width, uint32_t height, const void *data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		VulkanTexture2D(const std::string &name, const std::string &filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		VulkanTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height);
		VulkanTexture2D();
		~VulkanTexture2D();

		auto update(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void *buffer) -> void override;

		inline auto bind(uint32_t slot = 0) const -> void override{};
		inline auto unbind(uint32_t slot = 0) const -> void override{};
		inline auto setData(const void *pixels) -> void override{};

		inline auto getHandle() -> void * override
		{
			return (void *) this;
		};

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}
		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getMipMapLevels() const -> uint32_t override
		{
			return mipLevels;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return fileName;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::Color;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return parameters.format;
		}

		inline const auto getDescriptor() const
		{
			return &descriptor;
		}

		inline auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override
		{
			return (void *) getDescriptor();
		}

		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		}

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayout;
		}

		inline auto getDeviceMemory() const
		{
			return textureImageMemory;
		}
		inline auto getImageView() const
		{
			return textureImageView;
		}
		inline auto getSampler() const
		{
			return textureSampler;
		}

		inline auto getVkFormat() const
		{
			return vkFormat;
		}

		auto load() -> bool;
		auto updateDescriptor() -> void;
		auto buildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb, bool depth, bool samplerShadow, bool mipmap, bool image, uint32_t accessFlag) -> void override;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr) -> void override;

		auto getMipImageView(uint32_t mip) -> VkImageView;

		auto memoryBarrier(const CommandBuffer *cmd, uint32_t flags) -> void override;

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setName(const std::string &name) -> void override;

		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

	  private:
		auto createSampler() -> void;
		auto deleteSampler() -> void;

		std::string fileName;

		VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;

		uint32_t handle     = 0;
		uint32_t width      = 0;
		uint32_t height     = 0;
		uint32_t mipLevels  = 1;
		uint32_t layerCount = 1;

		bool deleteImage = false;

		const uint8_t *data = nullptr;

		TextureParameters  parameters;
		TextureLoadOptions loadOptions;

		VkImage               textureImage       = VK_NULL_HANDLE;
		VkImageView           textureImageView   = VK_NULL_HANDLE;
		VkDeviceMemory        textureImageMemory = VK_NULL_HANDLE;
		VkSampler             textureSampler     = VK_NULL_HANDLE;
		VkImageLayout         imageLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDescriptorImageInfo descriptor{};

		std::unordered_map<uint32_t, VkImageView> mipImageViews;

#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

	class VulkanTextureDepth : public TextureDepth, public VkTexture
	{
	  public:
		VulkanTextureDepth(uint32_t width, uint32_t height, bool stencil, const CommandBuffer *commandBuffer);
		~VulkanTextureDepth();

		auto bind(uint32_t slot = 0) const -> void override{};
		auto unbind(uint32_t slot = 0) const -> void override{};
		auto resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer) -> void override;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr) -> void override;

		inline auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override
		{
			return (void *) &descriptor;
		}
		virtual auto getHandle() -> void *
		{
			return (void *) this;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		}

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayout;
		}

		inline const auto getDeviceMemory() const
		{
			return textureImageMemory;
		}
		inline const auto getImageView() const
		{
			return textureImageView;
		}
		inline const auto getSampler() const
		{
			return textureSampler;
		}

		inline const auto getDescriptor() const
		{
			return &descriptor;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}
		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}
		auto updateDescriptor() -> void;

		inline auto getVkFormat() const
		{
			return vkFormat;
		}

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setName(const std::string &name) -> void override;
		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

	  protected:
		auto init(const CommandBuffer *commandBuffer = nullptr) -> void;
		auto release() -> void;

	  private:
		bool                  stencil = false;
		uint32_t              width   = 0;
		uint32_t              height  = 0;
		uint32_t              handle{};
		TextureFormat         format = TextureFormat::DEPTH;
		VkFormat              vkFormat;
		VkImageLayout         imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImage               textureImage{};
		VkDeviceMemory        textureImageMemory{};
		VkImageView           textureImageView{};
		VkSampler             textureSampler{};
		VkDescriptorImageInfo descriptor{};
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

	class VulkanTextureCube : public TextureCube, public VkTexture
	{
	  public:
		VulkanTextureCube(uint32_t size, TextureFormat format = TextureFormat::RGBA8, int32_t numMips = 1);
		VulkanTextureCube(const std::string &filePath);
		VulkanTextureCube(const std::array<std::string, 6> &files);
		VulkanTextureCube(const std::vector<std::string> &files, uint32_t mips, const TextureParameters &params, const TextureLoadOptions &loadOptions, const InputFormat &format);

		~VulkanTextureCube();
		auto bind(uint32_t slot = 0) const -> void override{};
		auto unbind(uint32_t slot = 0) const -> void override{};

		inline auto getHandle() -> void * override
		{
			return (void *) this;
		}

		auto generateMipmap(const CommandBuffer *commandBuffer) -> void override;

		auto updateDescriptor() -> void;
		auto load(uint32_t mips) -> void;

		auto update(const CommandBuffer *commandBuffer, FrameBuffer *framebuffer, int32_t cubeIndex, int32_t mipmapLevel = 0) -> void override;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr) -> void override;

		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		}

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayout;
		}

		inline auto getDeviceMemory() const
		{
			return textureImageMemory;
		}
		inline auto getImageView() const
		{
			return textureImageView;
		}
		inline auto getSampler() const
		{
			return textureSampler;
		}
		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}
		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}
		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getMipMapLevels() const -> uint32_t override
		{
			return numMips;
		}
		inline auto getType() const -> TextureType override
		{
			return TextureType::Cube;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return parameters.format;
		}

		inline auto getSize() const -> uint32_t override
		{
			return size;
		}

		inline auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override
		{
			return (void *) &descriptor;
		}

		inline auto getVkFormat() const
		{
			return vkFormat;
		}

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setName(const std::string &name) -> void override;

		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

	  private:
		auto init() -> void;

		VkFormat    vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
		std::string name;
		std::string files[6];

		uint32_t handle = 0;

		uint32_t width   = 0;
		uint32_t height  = 0;
		uint32_t size    = 0;
		uint32_t numMips = 0;

		TextureParameters  parameters;
		TextureLoadOptions loadOptions;

		std::vector<uint8_t> data;

		VkImage               textureImage       = nullptr;
		VkImageLayout         imageLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDeviceMemory        textureImageMemory = nullptr;
		VkImageView           textureImageView   = nullptr;
		VkSampler             textureSampler;
		VkDescriptorImageInfo descriptor;

		bool deleteImg = true;
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

	class VulkanTextureDepthArray : public TextureDepthArray, public VkTexture
	{
	  public:
		VulkanTextureDepthArray(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer);
		~VulkanTextureDepthArray();

		auto bind(uint32_t slot = 0) const -> void override{};
		auto unbind(uint32_t slot = 0) const -> void override{};
		auto resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) -> void override;

		auto getHandle() -> void * override
		{
			return (void *) this;
		}

		inline auto getImageView(int32_t index) const
		{
			return imageViews[index];
		}
		inline auto getSampler() const
		{
			return textureSampler;
		}
		inline auto getDescriptor() const
		{
			return &descriptor;
		}

		inline auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override
		{
			return (void *) &descriptor;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}
		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}
		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::DepthArray;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

		inline auto getCount() const
		{
			return count;
		}

		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		}

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayout;
		}

		auto getHandleArray(uint32_t index) -> void * override;
		auto updateDescriptor() -> void;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void override;

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setName(const std::string &name) -> void override;

		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

	  protected:
		auto init(const CommandBuffer *commandBuffer) -> void override;
		auto release() -> void;

	  private:
		uint32_t      handle{};
		uint32_t      width  = 0;
		uint32_t      height = 0;
		uint32_t      count  = 0;
		TextureFormat format;

		VkImageLayout            imageLayout;
		VkImage                  textureImage{};
		VkDeviceMemory           textureImageMemory{};
		VkImageView              textureImageView{};
		VkSampler                textureSampler{};
		VkDescriptorImageInfo    descriptor{};
		std::vector<VkImageView> imageViews;
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

	class VulkanTexture3D : public Texture3D, public VkTexture
	{
	  public:
		VulkanTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters, TextureLoadOptions loadOptions, const void *data = nullptr);
		~VulkanTexture3D();

		auto init(uint32_t width, uint32_t height, uint32_t depth, const void *data = nullptr) -> void;
		auto bind(uint32_t slot = 0) const -> void override;
		auto unbind(uint32_t slot = 0) const -> void override;
		auto generateMipmaps(const CommandBuffer *cmd) -> void override;
		auto bindImageTexture(uint32_t unit, bool read, bool write, uint32_t level, uint32_t layer, TextureFormat format = TextureFormat::NONE) -> void override;
		auto buildTexture3D(TextureFormat format, uint32_t width, uint32_t height, uint32_t depth) -> void override;

		auto updateMipmap(const CommandBuffer *cmd, uint32_t mipLevel, const void *buffer) -> void override;
		
		auto update(const CommandBuffer *cmd, const void *buffer, uint32_t x = 0, uint32_t y = 0, uint32_t z = 0) -> void override;

		virtual auto getFilePath() const -> const std::string & override
		{
			return filePath;
		};

		inline auto getHandle() -> void * override
		{
			return (void *) this;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getDepth() const -> uint32_t override
		{
			return height;
		}
		inline auto getFormat() const -> TextureFormat override
		{
			return parameters.format;
		}

		auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override;

		auto clear(const CommandBuffer *commandBuffer) -> void override;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr) -> void override;
		auto transitionImage2(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer = nullptr, int32_t mipLevel = -1) -> void;

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayouts[0];
		};
		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		};

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setName(const std::string &name) -> void override;

		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

	  private:
		auto deleteSampler() -> void;
		auto updateDescriptor() -> void;

		uint32_t           width     = 0;
		uint32_t           height    = 0;
		uint32_t           depth     = 0;
		uint32_t           mipLevels = 1;
		TextureParameters  parameters;
		TextureLoadOptions loadOptions;
		std::string        filePath;
		VkFormat           vkFormat             = VK_FORMAT_R8G8B8A8_UNORM;
		VkFormat           currentFormat        = VK_FORMAT_UNDEFINED;
		VkImage            textureImage         = nullptr;
		VkDeviceMemory     textureImageMemory   = nullptr;
		VkSampler          textureSampler       = nullptr;
		VkSampler          linearTextureSampler = nullptr;

		VkDescriptorImageInfo      descriptor;
		std::vector<VkImageView>   mipmapVies;
		std::vector<VkImageLayout> imageLayouts;

		VkImageView   textureImageView = nullptr;
		VkImageLayout imageLayout      = VK_IMAGE_LAYOUT_UNDEFINED;

		//TextureFormat * 100 + mipLevel
		std::unordered_map<size_t, VkImageView> formatToLayout;

#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

	class VulkanTexture2DArray : public Texture2DArray, public VkTexture
	{
	  public:
		VulkanTexture2DArray(uint32_t width, uint32_t height, uint32_t count, TextureFormat format, TextureParameters parameters, const CommandBuffer *commandBuffer);
		~VulkanTexture2DArray();

		auto bind(uint32_t slot = 0) const -> void override{};
		auto unbind(uint32_t slot = 0) const -> void override{};
		auto resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) -> void override;

		auto getHandle() -> void * override
		{
			return (void *) this;
		}

		inline auto getImageView(int32_t index) const
		{
			return imageViews[index];
		}
		inline auto getSampler() const
		{
			return textureSampler;
		}
		inline auto getDescriptor() const
		{
			return &descriptor;
		}

		inline auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void * override
		{
			return (void *) &descriptor;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

		inline auto getCount() const
		{
			return count;
		}

		inline auto getImage() const -> VkImage override
		{
			return textureImage;
		}

		inline auto getImageLayout() const -> VkImageLayout override
		{
			return imageLayout;
		}

		auto getHandleArray(uint32_t index) -> void * override;
		auto updateDescriptor() -> void;

		auto transitionImage(VkImageLayout newLayout, const VulkanCommandBuffer *commandBuffer) -> void override;

		auto toIntID() const -> const uint64_t override
		{
			return (uint64_t) textureImage;
		};

		auto setImageLayout(const VkImageLayout &layout) -> void override
		{
			imageLayout = layout;
		}

		auto setName(const std::string &name) -> void override;

	  protected:
		auto init(const CommandBuffer *commandBuffer) -> void override;
		auto release() -> void;

	  private:
		uint32_t          handle{};
		uint32_t          width  = 0;
		uint32_t          height = 0;
		uint32_t          count  = 0;
		TextureFormat     format;
		TextureParameters parameters;

		VkImageLayout            imageLayout;
		VkImage                  textureImage{};
		VkDeviceMemory           textureImageMemory{};
		VkImageView              textureImageView{};
		VkSampler                textureSampler{};
		VkDescriptorImageInfo    descriptor{};
		std::vector<VkImageView> imageViews;
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
#endif
	};

};        // namespace maple
