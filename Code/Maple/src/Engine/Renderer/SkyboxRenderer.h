//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Renderer.h"
#include "IoC/SystemBuilder.h"
#include <IconsMaterialDesignIcons.h>
namespace maple
{
	class PrefilterRenderer;

	namespace skybox_renderer
	{
		namespace SkyboxId
		{
			enum Id
			{
				CubeMap,
				Prefilter,
				Irradiance,
				Length
			};

			static const char *Names[] = {
			    "CubeMap",
			    "Prefilter",
			    "Irradiance",
			    nullptr};
		}        // namespace SkyboxId

		namespace global::component
		{
			struct SkyboxData
			{
				std::shared_ptr<Shader>        skyboxShader;
				std::shared_ptr<Pipeline>      pipeline;
				std::shared_ptr<DescriptorSet> descriptorSet;
				std::shared_ptr<Mesh>          skyboxMesh;

				bool pseudoSky = false;

				std::shared_ptr<Texture> skybox;
				std::shared_ptr<Texture> environmentMap;
				std::shared_ptr<Texture> irradianceSH;

				glm::mat4 projView = glm::mat4(1);

				std::shared_ptr<PrefilterRenderer> prefilterRenderer;

				int32_t cubeMapMode  = 0;
				float   cubeMapLevel = 0;
				bool    dirty        = false;
			};
		}        // namespace component

		auto registerSkyboxRenderer(SystemQueue &begin, SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void;
	};
}        // namespace maple
