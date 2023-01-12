//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Definitions.h"
#include <glm/vec4.hpp>

namespace maple
{
	enum class CubeFace : int32_t
	{
		PositiveX = 0,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ
	};

	struct FrameBufferInfo
	{
		uint32_t                              width;
		uint32_t                              height;
		uint32_t                              layer     = 0;
		uint32_t                              msaaLevel = 0;
		bool                                  screenFBO = false;
		std::vector<std::shared_ptr<Texture>> attachments;
		std::shared_ptr<RenderPass>           renderPass;
	};

	class MAPLE_EXPORT FrameBuffer
	{
	  public:
		virtual ~FrameBuffer() = default;

		static auto create(const FrameBufferInfo &info) -> std::shared_ptr<FrameBuffer>;

		virtual auto validate() -> void{};

		virtual auto bind(uint32_t width, uint32_t height) const -> void                                                                = 0;
		virtual auto bind() const -> void                                                                                               = 0;
		virtual auto unbind() const -> void                                                                                             = 0;
		virtual auto clear() -> void                                                                                                    = 0;
		virtual auto addTextureAttachment(TextureFormat format, const std::shared_ptr<Texture> &texture) -> void                        = 0;
		virtual auto addCubeTextureAttachment(TextureFormat format, CubeFace face, const std::shared_ptr<TextureCube> &texture) -> void = 0;
		virtual auto addShadowAttachment(const std::shared_ptr<Texture> &texture) -> void                                               = 0;
		virtual auto addTextureLayer(int32_t index, const std::shared_ptr<Texture> &texture) -> void                                    = 0;
		virtual auto generateFramebuffer() -> void                                                                                      = 0;
		virtual auto getWidth() const -> uint32_t                                                                                       = 0;
		virtual auto getHeight() const -> uint32_t                                                                                      = 0;
		virtual auto setClearColor(const glm::vec4 &color) -> void                                                                      = 0;
		virtual auto getColorAttachment(int32_t id = 0) const -> std::shared_ptr<Texture>                                               = 0;
	};
}        // namespace maple
