//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "IO/IResource.h"
#include "Others/Console.h"
#include "Definitions.h"
#include <string>

namespace maple
{
	class MAPLE_EXPORT Texture : public IResource
	{
	  public:
		virtual ~Texture()
		{
		}

		using Ptr = std::shared_ptr<Texture>;

		virtual auto bind(uint32_t slot = 0) const -> void      = 0;
		virtual auto unbind(uint32_t slot = 0) const -> void    = 0;
		virtual auto getFilePath() const -> const std::string & = 0;
		virtual auto getHandle() -> void *                      = 0;
		virtual auto getWidth() const -> uint32_t               = 0;
		virtual auto getHeight() const -> uint32_t              = 0;
		virtual auto getType() const -> TextureType             = 0;
		virtual auto getFormat() const -> TextureFormat         = 0;

		virtual auto memoryBarrier(const CommandBuffer *cmd, uint32_t flags) -> void;
		// rwFlag 0 1 2 -> r w rw

		virtual auto bindImageTexture(uint32_t unit, bool read = false, bool write = false, uint32_t level = 0, uint32_t layer = 0, TextureFormat format = TextureFormat::NONE) -> void{};

		virtual auto isUpdated() const -> bool
		{
			return updated;
		}

		inline auto setUpdate(bool update)
		{
			return updated = update;
		}

		virtual auto getSize() const -> uint32_t
		{
			return 0;
		}
		virtual auto getMipMapLevels() const -> uint32_t
		{
			return 0;
		}

		virtual auto getDescriptorInfo(int32_t mipLvl = 0, TextureFormat format = TextureFormat::RGBA8) -> void *
		{
			return getHandle();
		}

		inline static auto isDepthStencilFormat(TextureFormat format)
		{
			return format == TextureFormat::DEPTH_STENCIL;
		}

		inline static auto isDepthFormat(TextureFormat format)
		{
			return format == TextureFormat::DEPTH;
		}

		inline static bool isStencilFormat(TextureFormat format)
		{
			return format == TextureFormat::STENCIL;
		}

		virtual auto setName(const std::string &name) -> void
		{
			this->name = name;
		};

		virtual auto getName() const -> const std::string &
		{
			return name;
		};

		virtual auto getResourceType() const -> FileType override
		{
			return FileType::Texture;
		};

		virtual auto getPath() const -> std::string override
		{
			return "";
		}

		virtual auto toIntID() const -> const uint64_t
		{
			return 0;
		};

		inline auto getId() const
		{
			return id;
		}

	  public:
		static auto getStrideFromFormat(TextureFormat format) -> uint8_t;
		static auto bitsToTextureFormat(uint32_t bits) -> TextureFormat;
		static auto calculateMipMapCount(uint32_t width, uint32_t height) -> uint32_t;

	  protected:
		uint16_t    flags = 0;
		std::string name;
		uint32_t    id      = 0;
		bool        updated = true;
	};

	class MAPLE_EXPORT Texture2D : public Texture
	{
	  public:
		virtual auto setData(const void *data) -> void = 0;

		using Ptr = std::shared_ptr<Texture2D>;

	  public:
		static auto  getDefaultTexture() -> std::shared_ptr<Texture2D>;
		static auto  create() -> std::shared_ptr<Texture2D>;
		static auto  create(uint32_t width, uint32_t height, void *data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions()) -> std::shared_ptr<Texture2D>;
		static auto  create(const std::string &name, const std::string &filePath, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions()) -> std::shared_ptr<Texture2D>;
		virtual auto update(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void *buffer) -> void = 0;

		virtual auto buildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb = false, bool depth = false, bool samplerShadow = false, bool mipmap = false, bool image = false, uint32_t accessFlag = 0) -> void = 0;

		virtual auto generateMipmaps(const CommandBuffer *cmd) -> void{};

		inline auto getType() const -> TextureType override
		{
			return TextureType::Color;
		};
		static auto copy(const Texture2D::Ptr &from, Texture2D::Ptr &to, const CommandBuffer *cmdBuffer) -> void;
	};

	class MAPLE_EXPORT Texture3D : public Texture2D
	{
	  public:
		static auto create(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters = {}, TextureLoadOptions loadOptions = {}, const void *data = nullptr) -> std::shared_ptr<Texture3D>;

		virtual auto update(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void *buffer) -> void override
		{}

		virtual auto update(const CommandBuffer *cmd, const void *buffer, uint32_t x = 0, uint32_t y = 0, uint32_t z = 0 ) -> void
		{
			MAPLE_ASSERT(false, "");
		}

		virtual auto updateMipmap(const CommandBuffer *cmd, uint32_t mipLevel, const void *buffer) -> void
		{
			MAPLE_ASSERT(false, "");
		}

		virtual auto buildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb = false, bool depth = false, bool samplerShadow = false, bool mipmap = false, bool image = false, uint32_t accessFlag = 0) -> void
		{}

		virtual auto buildTexture3D(TextureFormat format, uint32_t width, uint32_t height, uint32_t depth) -> void
		{}

		virtual auto setData(const void *data) -> void override
		{}

		virtual auto generateMipmaps(const CommandBuffer *cmd) -> void = 0;

		inline auto getType() const -> TextureType override
		{
			return TextureType::Color3D;
		};

		virtual auto clear(const CommandBuffer *commandBuffer) -> void{};

		virtual auto getDepth() const -> uint32_t = 0;
	};

	class MAPLE_EXPORT TextureCube : public Texture
	{
	  protected:
		enum class InputFormat
		{
			VERTICAL_CROSS,
			HORIZONTAL_CROSS
		};

	  public:
		static auto create(uint32_t size) -> std::shared_ptr<TextureCube>;
		static auto create(uint32_t size, TextureFormat format, int32_t numMips) -> std::shared_ptr<TextureCube>;
		static auto createFromFile(const std::string &filePath) -> std::shared_ptr<TextureCube>;
		static auto createFromFiles(const std::array<std::string, 6> &files) -> std::shared_ptr<TextureCube>;
		static auto createFromVCross(const std::vector<std::string> &files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat = InputFormat::VERTICAL_CROSS) -> std::shared_ptr<TextureCube>;

		virtual auto update(const CommandBuffer *commandBuffer, FrameBuffer *framebuffer, int32_t cubeIndex, int32_t mipmapLevel = 0) -> void = 0;

		virtual auto generateMipmap(const CommandBuffer *commandBuffer) -> void = 0;
		inline auto  getType() const -> TextureType override
		{
			return TextureType::Cube;
		};
	};

	class MAPLE_EXPORT TextureDepth : public Texture
	{
	  public:
		static auto create(uint32_t width, uint32_t height, bool stencil = false, const CommandBuffer *commandBuffer = nullptr) -> std::shared_ptr<TextureDepth>;
		inline auto getType() const -> TextureType override
		{
			return TextureType::Depth;
		};
		virtual auto generateMipmaps(const CommandBuffer *cmd) -> void{};
		virtual auto resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer = nullptr, int32_t mipmap = 1) -> void = 0;
	};

	class MAPLE_EXPORT TextureDepthArray : public Texture
	{
	  public:
		static auto create(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer = nullptr) -> std::shared_ptr<TextureDepthArray>;

		virtual auto init(const CommandBuffer *commandBuffer = nullptr) -> void                                                    = 0;
		virtual auto resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer = nullptr) -> void = 0;
		virtual auto getHandleArray(uint32_t index) -> void *
		{
			return getHandle();
		};
		inline auto getType() const -> TextureType override
		{
			return TextureType::DepthArray;
		};
	};

	class MAPLE_EXPORT Texture2DArray : public Texture
	{
	  public:
		static auto create(uint32_t width, uint32_t height, uint32_t count,
		                   TextureFormat format = TextureFormat::RGBA8, TextureParameters parameters = {}, const CommandBuffer *commandBuffer = nullptr) -> std::shared_ptr<Texture2DArray>;

		virtual auto init(const CommandBuffer *commandBuffer = nullptr) -> void                                                    = 0;
		virtual auto resize(uint32_t width, uint32_t height, uint32_t count, const CommandBuffer *commandBuffer = nullptr) -> void = 0;
		virtual auto getHandleArray(uint32_t index) -> void *
		{
			return getHandle();
		};
		inline auto getType() const -> TextureType override
		{
			return TextureType::Color2DArray;
		};
	};
}        // namespace maple
