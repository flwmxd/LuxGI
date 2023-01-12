//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Texture.h"
#include "Others/Console.h"

#ifdef MAPLE_OPENGL
#	include "RHI/OpenGL/GLTexture.h"
#endif        // MAPLE_OPENGL

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanTexture.h"
#endif        // MAPLE_OPENGL

#include "IO/Loader.h"
#include "Engine/Renderer/Renderer.h"

namespace maple
{
	auto Texture::memoryBarrier(const CommandBuffer *cmd, uint32_t flags) -> void
	{
#ifdef MAPLE_OPENGL
		Renderer::memoryBarrier(cmd, flags);
#endif
	}

	auto Texture2D::copy(const Texture2D::Ptr &from, Texture2D::Ptr &to, const CommandBuffer *cmdBuffer) -> void
	{
#ifdef MAPLE_VULKAN
		VulkanHelper::copyTo(from, to, cmdBuffer);
#endif
	}

	auto Texture::getStrideFromFormat(TextureFormat format) -> uint8_t
	{
		switch (format)
		{
			case TextureFormat::RGB:
				return 3;
			case TextureFormat::RGBA:
				return 4;
			case TextureFormat::R8:
				return 1;
			case TextureFormat::RG8:
				return 2;
			case TextureFormat::RGB8:
				return 3;
			case TextureFormat::RGBA8:
				return 4;
			case TextureFormat::RG16F:
				return 4;
			default:
				return 0;
		}
	}

	auto Texture::bitsToTextureFormat(uint32_t bits) -> TextureFormat
	{
		switch (bits)
		{
			case 8:
				return TextureFormat::R8;
			case 16:
				return TextureFormat::RG8;
			case 24:
				return TextureFormat::RGB8;
			case 32:
				return TextureFormat::RGBA8;
			case 48:
				return TextureFormat::RGB16;
			case 64:
				return TextureFormat::RGBA16;
			default:
				MAPLE_ASSERT(false, "Unsupported image bit-depth!");
				return TextureFormat::RGB8;
		}
	}

	auto Texture::calculateMipMapCount(uint32_t width, uint32_t height) -> uint32_t
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
			levels++;
		return levels;
	}

	//###################################################

	auto Texture2D::create() -> std::shared_ptr<Texture2D>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTexture2D>();
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTexture2D>();
#endif        // MAPLE_OPENGL
	}

	auto Texture2D::create(uint32_t width, uint32_t height, void *data, TextureParameters parameters, TextureLoadOptions loadOptions) -> std::shared_ptr<Texture2D>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTexture2D>(width, height, data, parameters, loadOptions);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTexture2D>(width, height, data, parameters, loadOptions);
#endif        // MAPLE_OPENGL
	}

	auto Texture2D::create(const std::string &name, const std::string &filePath, TextureParameters parameters, TextureLoadOptions loadOptions) -> std::shared_ptr<Texture2D>
	{
#ifdef MAPLE_OPENGL
		return io::emplace<GLTexture2D>(filePath, name, filePath, parameters, loadOptions);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return io::emplace<VulkanTexture2D>(filePath, name, filePath, parameters, loadOptions);
#endif        // MAPLE_VULKAN
	}

	auto Texture2D::getDefaultTexture() -> std::shared_ptr<Texture2D>
	{
		static std::shared_ptr<Texture2D> defaultTexture = create("default", "textures/default.png");
		return defaultTexture;
	}
	//###################################################

	auto TextureDepth::create(uint32_t width, uint32_t height, bool stencil, const CommandBuffer *commandBuffer) -> std::shared_ptr<TextureDepth>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureDepth>(width, height, stencil);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureDepth>(width, height, stencil, commandBuffer);
#endif        // MAPLE_VULKAN
	}
	///#####################################################################################
	auto TextureCube::create(uint32_t size) -> std::shared_ptr<TextureCube>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureCube>(size);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureCube>(size);
#endif        // MAPLE_VULKAN
	}

	auto TextureCube::create(uint32_t size, TextureFormat format, int32_t numMips) -> std::shared_ptr<TextureCube>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureCube>(size, format, numMips);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureCube>(size, format, numMips);
#endif        // MAPLE_VULKAN
	}

	auto TextureCube::createFromFile(const std::string &filePath) -> std::shared_ptr<TextureCube>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureCube>(filePath);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureCube>(filePath);
#endif        // MAPLE_VULKAN
	}

	auto TextureCube::createFromFiles(const std::array<std::string, 6> &files) -> std::shared_ptr<TextureCube>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureCube>(files);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureCube>(files);
#endif        // MAPLE_VULKAN
	}

	auto TextureCube::createFromVCross(const std::vector<std::string> &files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat format) -> std::shared_ptr<TextureCube>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureCube>(files, mips, params, loadOptions, format);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureCube>(files, mips, params, loadOptions, format);
#endif        // MAPLE_VULKAN
	}

	///#####################################################################################
	auto TextureDepthArray::create(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer) -> std::shared_ptr<TextureDepthArray>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTextureDepthArray>(width, height, count);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTextureDepthArray>(width, height, count, commandBuffer);
#endif        // MAPLE_VULKAN
	}

	auto Texture3D::create(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters, TextureLoadOptions loadOptions, const void *data) -> std::shared_ptr<Texture3D>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTexture3D>(width, height, depth, parameters, loadOptions,data);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTexture3D>(width, height, depth, parameters, loadOptions, data);
#endif        // MAPLE_VULKAN
	}

	auto Texture2DArray::create(uint32_t width, uint32_t height, uint32_t count, TextureFormat format, TextureParameters parameters, const CommandBuffer *commandBuffer) -> std::shared_ptr<Texture2DArray>
	{
#ifdef MAPLE_OPENGL
		return std::make_shared<GLTexture2DArray>(width, height, count, format, parameters);
#endif        // MAPLE_OPENGL
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanTexture2DArray>(width, height, count, format, parameters, commandBuffer);
#endif        // MAPLE_VULKAN
	}
}        // namespace maple
