//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <glm/glm.hpp>

namespace maple
{
	namespace component
	{
		enum class LightType
		{
			DirectionalLight = 0,
			SpotLight        = 1,
			PointLight       = 2,
			EnvironmentLight = 3,
			AreaLight        = 4
		};

		struct LightData
		{
			glm::vec4 color     = glm::vec4(1.f, 1.f, 1.f, 1.f);        // 16
			glm::vec4 position  = {};
			glm::vec4 direction = {};
			//align to 16 bytes
			float intensity = 1.f;
			float radius    = 0.1f;
			float type      = 0.f;
			float angle     = 0.f;

			SERIALIZATION(color, position, direction, intensity, type, angle);
		};

		struct Light
		{
			LightData lightData;
			bool      showFrustum = false;
			bool      castShadow  = false;

			SERIALIZATION(lightData, showFrustum, castShadow);
		};

		inline auto isDirectionalLight(const component::Light &light)
		{
			return static_cast<LightType>(light.lightData.type) == LightType::DirectionalLight;
		}

		inline auto isPointLight(const component::Light &light)
		{
			return static_cast<LightType>(light.lightData.type) == LightType::PointLight;
		}

		inline auto isSpotLight(const component::Light &light)
		{
			return static_cast<LightType>(light.lightData.type) == LightType::SpotLight;
		}
	}        // namespace component
};           // namespace maple
