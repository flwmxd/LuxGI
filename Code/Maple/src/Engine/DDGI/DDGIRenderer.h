//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "IoC/SystemBuilder.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Texture.h"
#include <glm/glm.hpp>

namespace maple
{
	namespace ddgi
	{
		constexpr uint32_t IrradianceOctSize = 8;
		constexpr uint32_t DepthOctSize      = 16;

		namespace component
		{
			struct DDGIUniform
			{
				glm::vec4  startPosition;
				glm::vec4  step;//align
				glm::ivec4 probeCounts;

				float maxDistance    = 6.f;
				float depthSharpness = 50.f;
				float hysteresis     = 0.98f;
				float normalBias     = 1.0f;

				float   ddgiGamma        = 5.f;
				int32_t irradianceProbeSideLength = IrradianceOctSize;
				int32_t irradianceTextureWidth;
				int32_t irradianceTextureHeight;

				int32_t depthProbeSideLength = DepthOctSize;
				int32_t depthTextureWidth;
				int32_t depthTextureHeight;
				int32_t raysPerProbe = 128;
			};

			struct IrradianceVolume
			{
				float   probeDistance           = 1.5f;
				bool    infiniteBounce          = true;
				int32_t raysPerProbe            = 256;
				float   hysteresis              = 0.98f;
				float   intensity               = 1.0f;
				float   normalBias              = 0.1f;
				float   depthSharpness          = 50.f;
				float   ddgiGamma = 5.f;

				float             width;
				float             height;
				RaytraceScale::Id scale = RaytraceScale::Full;
				bool              enable = true;

				SERIALIZATION(probeDistance, infiniteBounce, raysPerProbe, hysteresis, intensity, normalBias, depthSharpness, ddgiGamma, scale);

				Texture::Ptr currentIrrdance;
				Texture::Ptr currentDepth;

				DescriptorSet::Ptr ddgiCommon;// set 5.
			};
		}        // namespace component

		auto registerDDGI(SystemQueue &begin, SystemQueue &render, std::shared_ptr<SystemBuilder> point) -> void;
	}        // namespace ddgi
}        // namespace maple