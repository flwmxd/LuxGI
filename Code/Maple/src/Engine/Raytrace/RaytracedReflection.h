//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include "RaytraceScale.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace raytraced_reflection
	{
		namespace component
		{
			struct RaytracedReflection
			{
				RaytraceScale::Id scale = RaytraceScale::Full;
				uint32_t          width;
				uint32_t          height;
				Texture2D::Ptr    output;
				bool              enable              = true;
				bool              approximateWithDDGI = true;
				bool              denoise             = true;


				//TemporalAccumulator
				float alpha        = 0.01f;
				float momentsAlpha = 0.2f;

				//AtrousFiler
				int32_t radius     = 1;
				float   phiColor   = 10.0f;
				float   phiNormal  = 32.f;
				float   sigmaDepth = 1.f;

				float bias    = 0.5f;        //normal.
				float sdfBias = 1.f;         //normal.
				float trim    = 0.8f;


				SERIALIZATION(scale, alpha, momentsAlpha, radius, phiColor, phiNormal, sigmaDepth, bias, sdfBias, trim);
			};
		}        // namespace component
		auto registerRaytracedReflection(SystemQueue &update, SystemQueue &queue, std::shared_ptr<SystemBuilder> builder) -> void;
	};        // namespace raytraced_reflection
};            // namespace maple
