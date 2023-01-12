//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Renderer.h"

#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"
#include "IoC/SystemBuilder.h"

#include <memory>

namespace maple
{
	namespace component
	{
		struct RendererData;
	}

	namespace deferred
	{
		enum DeferredOutput
		{
			Default,
			Albedo,
			Metallic,
			Roughness,
			AO,
			SSAO,
			Normal,
			CascadeShadow,
			Depth,
			RaytracedShadow,
			IndirectLight,
			ShadowMap,
			ShadowMap1,
			ShadowMap2,
			ShadowMap3,
			LinearZ,
			WorldPos,
			Length
		};

		static const char *Names[] =
		    {
		        "Default",
		        "Albedo",
		        "Metallic",
		        "Roughness",
		        "AO",
		        "SSAO",
		        "Normal",
		        "CascadeShadow",
		        "Depth",
		        "RaytracedShadow",
		        "IndirectLight",
		        "ShadowMap",
		        "ShadowMap1",
		        "ShadowMap2",
		        "ShadowMap3",
		        "LinearZ",
		        "WorldPos",
		        nullptr};

		namespace global::component
		{
			struct MAPLE_EXPORT DeferredData
			{
				std::vector<RenderCommand>                  commandQueue;
				std::shared_ptr<Material>                   defaultMaterial;
				std::vector<std::shared_ptr<DescriptorSet>> descriptorColorSet;
				std::vector<std::shared_ptr<DescriptorSet>> descriptorLightSet;

				std::shared_ptr<Texture2D> preintegratedFG;
				std::shared_ptr<Shader>    deferredColorShader;        //stage 0 get all color information
				std::shared_ptr<Shader>    deferredLightShader;        //stage 1 process lighting
				std::shared_ptr<Shader>    stencilShader;

				std::shared_ptr<DescriptorSet> stencilDescriptorSet;

				std::shared_ptr<Mesh> screenQuad;

				bool depthTest = true;

				DeferredData();

				DeferredOutput deferredOut = DeferredOutput::Default;
			};
		}        // namespace global::component

	}        // namespace deferred

	namespace deferred_offscreen
	{
		auto registerGBufferRenderer(SystemQueue &begin, SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void;
	};        // namespace deferred_offscreen

	namespace deferred_lighting
	{
		auto registerDeferredLighting(SystemQueue &begin, SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void;
	};
}        // namespace maple
