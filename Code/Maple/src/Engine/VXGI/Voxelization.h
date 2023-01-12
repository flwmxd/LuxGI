//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Math/BoundingBox.h"
#include "RHI/Definitions.h"
#include <memory>

namespace maple
{
	class SystemBuilder;
	struct SystemQueue;

	namespace vxgi
	{
		namespace component
		{
			struct VoxelVolume
			{
				constexpr static int32_t voxelDimension = 256;
				constexpr static int32_t voxelVolume = voxelDimension * voxelDimension * voxelDimension;
				bool                     dirty = false;
				bool                     injectFirstBounce = true;
				bool                     enableIndirect = true;
				float                    volumeGridSize; 
				float                    voxelSize;
				float                    maxTracingDistance = 1.0f;
				float                    aoFalloff = 725.f;
				float                    samplingFactor = 0.5f;
				float                    bounceStrength = 1.f;
				float                    aoAlpha = 0.01;
				float                    traceShadowHit = 0.5f;

				SERIALIZATION(injectFirstBounce, enableIndirect, volumeGridSize, voxelSize, maxTracingDistance, aoFalloff, samplingFactor, bounceStrength, aoAlpha, traceShadowHit);
			};
		};        // namespace component
	}        // namespace vxgi
};           // namespace maple