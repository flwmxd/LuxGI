//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include <array>
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	enum class TextureFormat;
	enum GBufferTextures
	{
		COLOR,          //Main Render
		NORMALS,        //Deferred Render - World Space Normals
		PBR,
		EMISSIVE,

		COLOR_PONG,
		NORMALS_PONG,        //Deferred Render - World Space Normals
		PBR_PONG,
		EMISSIVE_PONG,
		SCREEN,
		INDIRECT_LIGHTING,
		LENGTH
	};

	static constexpr char *GBufferNames[] =
	    {
	        "Color",
	        "Normals",
	        "PBR",
	        "EMISSIVE",

	        "Color-Pong",
	        "Normals-Pong",
	        "PBR-Pong",
	        "Emissive-Pong",

	        "Screen",
	        "IndirectLighting",
	        nullptr};

	enum PingPong
	{
		Ping,
		Pong
	};

	class GBuffer
	{
	  public:
		GBuffer(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer = nullptr);

		inline auto getWidth() const
		{
			return width;
		}
		inline auto getHeight() const
		{
			return height;
		}
		auto resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer = nullptr) -> void;
		auto buildTexture(const CommandBuffer *commandBuffer = nullptr) -> void;

		inline auto getDepthBuffer()
		{
			return depthBuffer[index];
		}

		inline auto getDepthBufferPong()
		{
			return depthBuffer[1 - index];
		}

		auto getBuffer(uint32_t index, bool pong = false) -> std::shared_ptr<Texture2D>;

		inline auto getBufferWithoutPingPong(uint32_t index) -> std::shared_ptr<Texture2D> 
		{
			return screenTextures[index];
		}

		inline auto getFormat(uint32_t index) const
		{
			return formats[index];
		}

		inline auto pingPong() -> void 
		{
			index = 1 - index;
		}

		inline auto getIndex() const
		{
			return index;
		}

	  private:
		std::array<std::shared_ptr<Texture2D>, GBufferTextures::LENGTH> screenTextures;
		std::array<TextureFormat, GBufferTextures::LENGTH>              formats;
		std::shared_ptr<TextureDepth>                                   depthBuffer[2];

	  private:
		uint32_t width;
		uint32_t height;
		int32_t  index = PingPong::Ping;
	};
}        // namespace maple
