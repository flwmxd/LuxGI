//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include "RaytraceScale.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace raytraced_shadow
	{
		namespace component
		{
			struct RaytracedShadow
			{
				RaytraceScale::Id scale = RaytraceScale::Full;
				uint32_t          width;
				uint32_t          height;
				Texture2D::Ptr    output;
				float             showBias  = 0.5f;
				float             sdfBias  = 1.f;
				bool              denoise   = true;
				SERIALIZATION(scale, showBias);
			};
		}        // namespace component
		auto registerRaytracedShadow(SystemQueue &update, SystemQueue &queue, std::shared_ptr<SystemBuilder> builder) -> void;
	};        // namespace raytraced_shadow
};            // namespace maple
