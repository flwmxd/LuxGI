//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "AtmosphereRenderer.h"

#include "IO/File.h"

#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Scene/Component/Environment.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "Application.h"

#include "RendererData.h"

#include "IoC/Registry.h"
#include <vector>

namespace maple
{
	namespace atmosphere_pass
	{
		namespace begin_scene
		{
			inline auto system(ioc::Registry registry,
				component::AtmosphereData& data,
				const component::RendererData& renderData,
				const component::CameraView& cameraView)
			{
				for (auto [entity, atmosphere, light, transform] : registry.getRegistry().view<
					component::Environment,
					component::Light,
					component::Transform
				>().each())
				{
					if (atmosphere.pseudoSky)
					{
						auto dir = -light.lightData.direction;

						if (data.uniformObject.betaR != atmosphere.betaR || data.uniformObject.mieG != atmosphere.mieG || data.uniformObject.sunDir != dir || data.uniformObject.sunIntensity != light.lightData.intensity * 7.5f
							/*|| data.uniformObject.cameraPos != glm::vec4{cameraView.cameraTransform->getWorldPosition(), 1.f}*/)
						{
							data.uniformObject.betaR = atmosphere.betaR;
							data.uniformObject.mieG = atmosphere.mieG;
							data.uniformObject.sunDir = dir;
							data.uniformObject.sunIntensity = light.lightData.intensity * 7.5f;
							//data.uniformObject.cameraPos    = {cameraView.cameraTransform->getWorldPosition(), 1.f};
							data.descriptorSet->setUniformBufferData("UniformBufferObject", &data.uniformObject);
							data.descriptorSet->setTexture("uTransmittance", data.transmittance);
							data.descriptorSet->setTexture("uInscatter", data.inscatter);
							data.dirty = true;
						}
					}
				}
			}
		}        // namespace begin_scene

		namespace render_scene
		{
			inline auto system(component::AtmosphereData& data, const component::RendererData& renderData)
			{
				data.descriptorSet->update(renderData.commandBuffer);
			}
		}        // namespace render_scene
	}        // namespace atmosphere_pass

	namespace atmosphere_pass
	{
		auto registerAtmosphere(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<component::AtmosphereData>([](auto& data) {
				data.atmosphereShader = Shader::create("shaders/Atmosphere/Atmosphere.shader");
				data.descriptorSet = DescriptorSet::create({ 0, data.atmosphereShader.get() });
				memset(&data.uniformObject, 0, sizeof(component::AtmosphereData::UniformBufferObject));
				//data.uniformObject.earthPos = {0.0f, 6360010.0f, 0.0f};
				data.uniformObject.earthPos = { 0.0f, 6360010.0f, 0.0f };

				auto transData = File::read("atmosphere/transmittance.raw");
				data.transmittance = Texture2D::create(TransmittanceW, TransmittanceH, (void*)transData->data(), TextureParameters{ TextureFormat::RGBA32, TextureFilter::Linear, TextureWrap::ClampToEdge });

				auto irradianceData = File::read("atmosphere/irradiance.raw");
				data.irradiance = Texture2D::create(IrradianceW, IrradianceH, (void*)irradianceData->data(), TextureParameters{ TextureFormat::RGBA32, TextureFilter::Linear, TextureWrap::ClampToEdge });

				auto inscatterData = File::read("atmosphere/inscatter.raw");
				data.inscatter = Texture3D::create(InscatterMu_S * InscatterNu, InscatterMu, InscatterR, TextureParameters{ TextureFormat::RGBA32, TextureFilter::Linear, TextureWrap::ClampToEdge }, {}, (void*)inscatterData->data());

				data.shShader = Shader::create("shaders/Atmosphere/SphericalHarmonics.shader");
				data.shDescriptorSet = DescriptorSet::create({ 0, data.shShader.get() });

				data.shAddShader = Shader::create("shaders/Atmosphere/AddSH.shader");
				data.shAddDescriptorSet = DescriptorSet::create({ 0, data.shAddShader.get() });
				});

			builder->registerWithinQueue<begin_scene::system>(begin);
			builder->registerWithinQueue<render_scene::system>(renderer);
		}
	}        // namespace atmosphere_pass
};           // namespace maple
