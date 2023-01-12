///////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SkyboxRenderer.h"
#include "RHI/GPUProfile.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "AtmosphereRenderer.h"
#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "PrefilterRenderer.h"

#include "Scene/Component/Environment.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"
#include "Scene/System/EnvironmentModule.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "RendererData.h"

#include "Application.h"

#include <glm/gtc/type_ptr.hpp>

#include "IoC/Registry.h"

namespace maple
{
	namespace skybox_renderer
	{
		namespace begin_scene
		{
			inline auto system(
				ioc::Registry registry,
				global::component::SkyboxData& skyboxData,
				const maple::component::CameraView& cameraView)
			{
				for (auto [entity, envData] : registry.getRegistry().view<maple::component::Environment>().each())
				{
					skyboxData.dirty = skyboxData.prefilterRenderer->beginScene(envData);
					if (skyboxData.skybox != envData.environment)
					{
						skyboxData.environmentMap = envData.prefilteredEnvironment;
						skyboxData.irradianceSH = envData.irradianceSH;
						skyboxData.skybox = envData.environment;
					}
					auto inverseCamerm = cameraView.view;
					inverseCamerm[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
					skyboxData.projView = cameraView.proj * inverseCamerm;
				}

				if (registry.getRegistry().view<maple::component::Environment>().empty())
				{
					skyboxData.skybox = nullptr;
				}
			}
		}        // namespace begin_scene

		namespace render_scene
		{
			inline auto system(ioc::Registry registry,
				global::component::SkyboxData& skyboxData,
				capture_graph::component::RenderGraph& graph,
				const maple::component::RendererData& rendererData,
				const maple::component::CameraView& cameraView,
				maple::component::AtmosphereData* atmosphereData)
			{
				for (auto [entity, envData] : registry.getRegistry().view<maple::component::Environment>().each())
				{
					if (envData.filePath != "" && envData.equirectangularMap == nullptr)
						environment::init(envData, envData.filePath);

					GPUProfile("SkyBox Pass");
					if (skyboxData.dirty || (atmosphereData && atmosphereData->dirty))
					{
						skyboxData.prefilterRenderer->renderScene(rendererData.commandBuffer, graph, atmosphereData);
						skyboxData.dirty = false;
					}

					if (skyboxData.skybox == nullptr)
						return;

					PipelineInfo pipelineInfo{};
					pipelineInfo.shader = skyboxData.skyboxShader;

					pipelineInfo.polygonMode = PolygonMode::Fill;
					pipelineInfo.cullMode = CullMode::Front;
					pipelineInfo.transparencyEnabled = false;
					//pipelineInfo.clearTargets        = false;

					pipelineInfo.depthTarget = rendererData.gbuffer->getDepthBuffer();
					pipelineInfo.colorTargets[0] = rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN);

					auto skyboxPipeline = Pipeline::get(pipelineInfo, { skyboxData.descriptorSet }, graph);
					if (skyboxData.cubeMapMode == 0)
					{
						skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.skybox);
					}
					else if (skyboxData.cubeMapMode == 1)
					{
						skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.environmentMap); //prefilter
					}
					else if (skyboxData.cubeMapMode == 2)
					{
						//skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.irradianceMap);
					}
					skyboxData.descriptorSet->setUniform("UniformBufferObjectLod", "lodLevel", &skyboxData.cubeMapLevel);
					skyboxData.descriptorSet->update(rendererData.commandBuffer);

					skyboxPipeline->bind(rendererData.commandBuffer);
					auto& constants = skyboxData.skyboxShader->getPushConstants();
					constants[0].setValue("projView", glm::value_ptr(skyboxData.projView));
					skyboxData.skyboxShader->bindPushConstants(rendererData.commandBuffer, skyboxPipeline.get());

					Renderer::bindDescriptorSets(skyboxPipeline.get(), rendererData.commandBuffer, 0, { skyboxData.descriptorSet });
					Renderer::drawMesh(rendererData.commandBuffer, skyboxPipeline.get(), skyboxData.skyboxMesh.get());
					skyboxPipeline->end(rendererData.commandBuffer);
				}
			}
		}        // namespace render_scene
	}            // namespace skybox_renderer

	namespace skybox_renderer
	{
		auto registerSkyboxRenderer(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<global::component::SkyboxData>([](auto& skybox) {
				skybox.skyboxShader = Shader::create("shaders/Skybox.shader");
				skybox.descriptorSet = DescriptorSet::create({ 0, skybox.skyboxShader.get() });
				skybox.skyboxMesh = Mesh::createCube();

				skybox.prefilterRenderer = std::make_shared<PrefilterRenderer>();
				skybox.prefilterRenderer->init();
				});
			builder->registerWithinQueue<begin_scene::system>(begin);
			builder->registerWithinQueue<render_scene::system>(renderer);
		}
	};        // namespace skybox_renderer
};            // namespace maple
