//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/DescriptorSet.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"
#include "Renderer.h"

#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace
	{
		constexpr int32_t TransmittanceW = 256;
		constexpr int32_t TransmittanceH = 64;

		constexpr int32_t IrradianceW = 64;
		constexpr int32_t IrradianceH = 16;

		constexpr int32_t InscatterR    = 32;
		constexpr int32_t InscatterMu   = 128;
		constexpr int32_t InscatterMu_S = 32;
		constexpr int32_t InscatterNu   = 8;
	}        // namespace

	namespace component
	{
		struct AtmosphereData
		{
			struct UniformBufferObject
			{
				//glm::vec4 cameraPos;
				glm::vec3 earthPos;
				float     sunIntensity;
				glm::vec4 sunDir;
				glm::vec3 betaR;
				float     mieG;
			} uniformObject;

			std::shared_ptr<Shader>        atmosphereShader;
			std::shared_ptr<DescriptorSet> descriptorSet;
			Texture2D::Ptr                 transmittance;
			Texture2D::Ptr                 irradiance;
			Texture3D::Ptr                 inscatter;

			///compute SH
			std::shared_ptr<Shader>        shShader;
			std::shared_ptr<DescriptorSet> shDescriptorSet;
			//add SH
			std::shared_ptr<Shader>        shAddShader;
			std::shared_ptr<DescriptorSet> shAddDescriptorSet;

			bool dirty = true;
		};
	}        // namespace component

	namespace atmosphere_pass
	{
		auto registerAtmosphere(SystemQueue &begin, SystemQueue &renderer, std::shared_ptr<SystemBuilder> builder) -> void;
	}
}        // namespace maple
