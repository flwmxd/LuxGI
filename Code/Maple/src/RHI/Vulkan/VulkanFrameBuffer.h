//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Engine/Core.h"
#include "RHI/FrameBuffer.h"
#include "VulkanHelper.h"
#include <memory>

namespace maple
{
	class VulkanFrameBuffer : public FrameBuffer
	{
	  public:
		VulkanFrameBuffer(const FrameBufferInfo &info);
		~VulkanFrameBuffer();
		NO_COPYABLE(VulkanFrameBuffer);
		inline auto bind(uint32_t width, uint32_t height) const -> void{};
		inline auto bind() const -> void override{};
		inline auto unbind() const -> void override{};
		inline auto clear() -> void override{};
		inline auto addTextureAttachment(TextureFormat format, const std::shared_ptr<Texture> &texture) -> void override{};
		inline auto addCubeTextureAttachment(TextureFormat format, CubeFace face, const std::shared_ptr<TextureCube> &texture) -> void override{};
		inline auto addShadowAttachment(const std::shared_ptr<Texture> &texture) -> void override{};
		inline auto addTextureLayer(int32_t index, const std::shared_ptr<Texture> &texture) -> void override{};
		inline auto generateFramebuffer() -> void override{};
		inline auto setClearColor(const glm::vec4 &color) -> void override{};
		inline auto getColorAttachment(int32_t id = 0) const -> std::shared_ptr<Texture> override
		{
			return nullptr;
		};

		autoUnpack(buffer);

		inline auto &getFrameBufferInfo() const
		{
			return info;
		}
		inline auto getWidth() const -> uint32_t override
		{
			return info.width;
		};
		inline auto getHeight() const -> uint32_t override
		{
			return info.height;
		};

	  private:
		FrameBufferInfo info;
		VkFramebuffer   buffer = nullptr;
	};

};        // namespace maple
