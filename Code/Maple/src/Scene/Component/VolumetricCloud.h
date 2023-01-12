//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Engine/Core.h"

namespace maple
{
	namespace component
	{
		struct VolumetricCloud
		{
			float cloudSpeed = 450.0f;
			float coverage   = 0.45f;
			float crispiness = 40.f;
			float curliness  = .1f;
			float density    = 0.02f;
			float absorption = 0.35f;

			float earthRadius       = 600000.0f;
			float sphereInnerRadius = 5000.0f;
			float sphereOuterRadius = 17000.0f;

			float perlinFrequency = 0.8f;

			bool enableGodRays;
			bool enablePowder;
			bool postProcess;

			bool weathDirty = true;

			SERIALIZATION(cloudSpeed, coverage, crispiness, curliness, density, absorption, earthRadius, sphereInnerRadius, sphereOuterRadius, perlinFrequency, enableGodRays, enablePowder, postProcess);
		};
	};        // namespace component
};            // namespace maple
