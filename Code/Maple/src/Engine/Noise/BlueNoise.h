//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include "RHI/Texture.h"
#include "RHI/DescriptorSet.h"

namespace maple
{
	class SystemBuilder;

	namespace blue_noise
	{
		static constexpr const char *SOBOL_TEXTURE = "textures/blue_noise/sobol_256_4d.png";

		static constexpr const char *SCRAMBLING_RANKING_TEXTURES[] = {
		    "textures/blue_noise/scrambling_ranking_128x128_2d_1spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_2spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_4spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_8spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_16spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_32spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_64spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_128spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_256spp.png"};

		static constexpr int32_t LDR_LEN = 16;

		static constexpr const char *LDR_TEXTURES[LDR_LEN] = {
		    "textures/blue_noise/LDR_LLL1_0.png",
		    "textures/blue_noise/LDR_LLL1_1.png",
		    "textures/blue_noise/LDR_LLL1_2.png",
		    "textures/blue_noise/LDR_LLL1_3.png",
		    "textures/blue_noise/LDR_LLL1_4.png",
		    "textures/blue_noise/LDR_LLL1_5.png",
		    "textures/blue_noise/LDR_LLL1_6.png",
		    "textures/blue_noise/LDR_LLL1_7.png",
		    "textures/blue_noise/LDR_LLL1_8.png",
		    "textures/blue_noise/LDR_LLL1_9.png",
		    "textures/blue_noise/LDR_LLL1_10.png",
		    "textures/blue_noise/LDR_LLL1_11.png",
		    "textures/blue_noise/LDR_LLL1_12.png",
		    "textures/blue_noise/LDR_LLL1_13.png",
		    "textures/blue_noise/LDR_LLL1_14.png",
		    "textures/blue_noise/LDR_LLL1_15.png",
		};

		enum BlueNoiseSpp : uint8_t
		{
			Blue_Noise_1SPP,
			Blue_Noise_2SPP,
			Blue_Noise_4SPP,
			Blue_Noise_8SPP,
			Blue_Noise_16SPP,
			Blue_Noise_32SPP,
			Blue_Noise_64SPP,
			Blue_Noise_128SPP,
			Length
		};

		namespace global::component
		{
			struct BlueNoise
			{
				Texture2D::Ptr sobolSequence;
				Texture2D::Ptr scramblingRanking[BlueNoiseSpp::Length];
				Texture2D::Ptr ldrTextures[LDR_LEN];
			};
		}

		auto registerBlueNoiseModule(std::shared_ptr<SystemBuilder> builder) -> void;

		auto generateBlueNoiseRG16(uint32_t width, uint32_t height) -> Texture2D::Ptr;
	}
}