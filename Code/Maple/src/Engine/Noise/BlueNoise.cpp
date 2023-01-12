//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "BlueNoise.h"
#include "IoC/SystemBuilder.h"
#include "generate_blue_noise.h"

namespace maple
{
	namespace blue_noise
	{
		auto registerBlueNoiseModule(std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<blue_noise::global::component::BlueNoise>([](auto &noise) {
				noise.sobolSequence = Texture2D::create(blue_noise::SOBOL_TEXTURE, blue_noise::SOBOL_TEXTURE);
				for (int32_t i = 0; i < blue_noise ::BlueNoiseSpp::Length; i++)
				{
					noise.scramblingRanking[i] = Texture2D::create(blue_noise::SCRAMBLING_RANKING_TEXTURES[i], blue_noise::SCRAMBLING_RANKING_TEXTURES[i]);
				}

				for (int32_t i = 0; i < blue_noise::LDR_LEN; i++)
				{
					noise.ldrTextures[i] = Texture2D::create(LDR_TEXTURES[i], LDR_TEXTURES[i], TextureParameters{TextureFormat::RGBA8, TextureFilter::Nearest, TextureWrap::Repeat});
				}
			});
		}

		auto generateBlueNoise(uint32_t width, uint32_t height, uint32_t channel) -> Texture2D::Ptr
		{
			BlueNoiseGenerator generator(width, height, 2);
			return Texture2D::create(width, height, generator.blue_noise.data(), {TextureFormat::RG16F, TextureFilter::Nearest, TextureWrap::Repeat});
		}
	}        // namespace blue_noise
}        // namespace maple