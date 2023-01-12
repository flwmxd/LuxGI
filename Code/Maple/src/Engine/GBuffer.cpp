//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GBuffer.h"
#include "Math/MathUtils.h"
#include "Others/Randomizer.h"

namespace maple
{
	GBuffer::GBuffer(uint32_t width, uint32_t height, const CommandBuffer* commandBuffer) :
		width(width),
		height(height)
	{
		formats[SCREEN] = TextureFormat::RGBA32;
		formats[COLOR] = TextureFormat::RGBA32;
		formats[NORMALS] = TextureFormat::RGBA32;
		formats[PBR] = TextureFormat::RGBA32;
		formats[EMISSIVE] = TextureFormat::R11G11B10;
		formats[COLOR_PONG] = TextureFormat::RGBA32;
		formats[NORMALS_PONG] = TextureFormat::RGBA32;
		formats[PBR_PONG] = TextureFormat::RGBA32;
		formats[EMISSIVE_PONG] = TextureFormat::R11G11B10;
		formats[INDIRECT_LIGHTING] = TextureFormat::RGBA32;
	}

	auto GBuffer::resize(uint32_t width, uint32_t height, const CommandBuffer* commandBuffer) -> void
	{
		this->width = width;
		this->height = height;
		buildTexture(commandBuffer);
	}

	auto GBuffer::buildTexture(const CommandBuffer* commandBuffer) -> void
	{
		if (depthBuffer[0] == nullptr)
		{
			for (auto i = 0; i < GBufferTextures::LENGTH; i++)
			{
				screenTextures[i] = Texture2D::create();
				screenTextures[i]->setName(GBufferNames[i]);
			}

			depthBuffer[0] = TextureDepth::create(width, height, true, commandBuffer);
			depthBuffer[0]->setName("GBuffer-Depth-Ping");

			depthBuffer[1] = TextureDepth::create(width, height, true, commandBuffer);
			depthBuffer[1]->setName("GBuffer-Depth-Pong");
		}

		for (int32_t i = COLOR; i < LENGTH; i++)
		{
			screenTextures[i]->buildTexture(formats[i], width, height, false, false, false);
		}
		depthBuffer[0]->resize(width, height, commandBuffer);
		depthBuffer[1]->resize(width, height, commandBuffer);
	}

	auto GBuffer::getBuffer(uint32_t i, bool pong) -> std::shared_ptr<Texture2D>
	{
		if (i <= EMISSIVE)
		{
			std::shared_ptr<Texture2D> textures[2] =
			{
				screenTextures[i],
				screenTextures[i + EMISSIVE + 1] };
			return pong ? textures[1 - index] : textures[index];
		}

		return screenTextures[i];
	}
}        // namespace maple
