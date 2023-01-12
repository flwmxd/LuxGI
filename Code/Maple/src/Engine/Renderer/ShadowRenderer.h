//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Engine/Renderer/Renderer.h"
#include "Math/Frustum.h"
#include "RHI/Shader.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace ShadowingMethod
	{
		enum Id
		{
			ShadowMap,
			TraceShadowCone,
			RaytracedShadow,
			Length
		};

		static const char *Names[] =
		    {
		        "ShadowMap",
		        "TraceShadowCone",
		        "RaytracedShadow",
		        nullptr};
	}        // namespace ShadowingMethod

	namespace component
	{
		struct ShadowMapData
		{
			float cascadeSplitLambda    = 0.995f;
			float sceneRadiusMultiplier = 1.4f;
			float lightSize             = 1.5f;
			float maxShadowDistance     = 400.0f;
			float shadowFade            = 40.0f;
			float cascadeTransitionFade = 3.0f;
			float initialBias           = 0.005f;
			bool  shadowMapsInvalidated = true;
			bool  dirty                 = true;

			maple::ShadowingMethod::Id shadowMethod = ShadowingMethod::RaytracedShadow;

			uint32_t  shadowMapNum                  = 4;
			uint32_t  shadowMapSize                 = SHADOWMAP_SiZE_MAX;
			glm::mat4 shadowProjView[SHADOWMAP_MAX] = {};
			glm::vec4 splitDepth[SHADOWMAP_MAX]     = {};
			glm::mat4 lightMatrix                   = glm::mat4(1.f);
			glm::mat4 shadowProj                    = glm::mat4(1.f);
			glm::vec3 lightDir                      = {};
			Frustum   cascadeFrustums[SHADOWMAP_MAX];

			std::vector<std::shared_ptr<DescriptorSet>> descriptorSet;
			std::vector<RenderCommand>         cascadeCommandQueue[SHADOWMAP_MAX];
			std::shared_ptr<Shader>            shader;
			std::shared_ptr<TextureDepthArray> shadowTexture;

			std::shared_ptr<Pipeline> pipeline;
		};
	}        // namespace component

	namespace shadow_map
	{
		auto registerShadowMap(SystemQueue &begin, SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void;
	};
};        // namespace maple