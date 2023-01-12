///////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DeferredOffScreenRenderer.h"
#include "RHI/CommandBuffer.h"
#include "RHI/GPUProfile.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Engine/DDGI/DDGIRenderer.h"

#include "Scene/Component/Component.h"
#include "Scene/Component/Environment.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "Engine/DDGI/MeshDistanceField.h"

#include "ShadowRenderer.h"
#include "SkyboxRenderer.h"

#include "Engine/Raytrace/RaytracedReflection.h"
#include "Engine/Raytrace/RaytracedShadow.h"

#include "Application.h"
#include "ImGui/ImGuiHelpers.h"
#include "Others/Randomizer.h"
#include "RendererData.h"
#include "IoC/Registry.h"
#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	namespace deferred::global::component
	{
		DeferredData::DeferredData()
		{
			deferredColorShader = Shader::create("shaders/DeferredColor.shader");
			deferredLightShader = Shader::create("shaders/DeferredLight.shader");
			commandQueue.reserve(1000);

			MaterialProperties properties;
			properties.albedoColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor = glm::vec4(0);
			properties.metallicColor = glm::vec4(0);
			properties.usingAlbedoMap = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap = 0.0f;
			properties.usingMetallicMap = 0.0f;

			defaultMaterial = std::make_shared<Material>(deferredColorShader, properties);
			defaultMaterial->createDescriptorSet();

			DescriptorInfo info{};

			descriptorColorSet.resize(3);
			descriptorLightSet.resize(1);

			info.shader = deferredColorShader.get();
			info.layoutIndex = 0;
			descriptorColorSet[0] = DescriptorSet::create(info);

			info.layoutIndex = 2;
			descriptorColorSet[2] = DescriptorSet::create(info);

			info.shader = deferredLightShader.get();
			info.layoutIndex = 0;
			descriptorLightSet[0] = DescriptorSet::create(info);
			screenQuad = Mesh::createQuad(true);

			preintegratedFG = Texture2D::create(
				"preintegrated",
				"textures/ibl_brdf_lut.png",
				{ TextureFormat::RG16F, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::ClampToEdge });
		}
	}        // namespace deferred::global::component

	namespace deferred_offscreen
	{
		inline auto beginScene(deferred::global::component::DeferredData& data,
			const component::ShadowMapData& shadowData,
			const component::CameraView& cameraView,
			const component::RendererData& renderData,
			const skybox_renderer::global::component::SkyboxData* skyboxData,
			ioc::Registry registry
			)
		{
			auto lightQuery = registry.getRegistry().view<component::Light, component::Transform>();
			auto env = registry.getRegistry().view<component::Environment>();
			auto ddgiGroup = registry.getRegistry().view<ddgi::component::IrradianceVolume>();
			auto meshQuery = registry.getRegistry().view<component::MeshRenderer, component::Transform>();

			data.commandQueue.clear();
			auto descriptorSet = data.descriptorColorSet[0];

			if (cameraView.cameraTransform == nullptr)
				return;

			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "projView", &cameraView.projView);
			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "view", &cameraView.view);
			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "projViewOld", &cameraView.projViewOld);

			data.descriptorColorSet[2]->setUniform("UBO", "view", &cameraView.view);
			data.descriptorColorSet[2]->setUniform("UBO", "nearPlane", &cameraView.nearPlane);
			data.descriptorColorSet[2]->setUniform("UBO", "farPlane", &cameraView.farPlane);

			data.defaultMaterial->bind(renderData.commandBuffer);

			component::Light* directionaLight = nullptr;

			component::LightData lights[32] = {};
			uint32_t             numLights = 0;

			{
				PROFILE_SCOPE("Get Light");
				for (auto [entity, light, transform] : lightQuery.each())
				{
					light.lightData.position = { transform.getWorldPosition(), 1.f };
					light.lightData.direction = { glm::normalize(transform.getWorldOrientation() * maple::FORWARD), light.lightData.direction.w };

					if (static_cast<component::LightType>(light.lightData.type) == component::LightType::DirectionalLight)
						directionaLight = &light;

					lights[numLights] = light.lightData;
					numLights++;
				}
			}

			int32_t ddgiEanble = 0;

			if (!ddgiGroup.empty())
			{
				auto [entity, ddgi] = *ddgiGroup.each().begin();
				ddgiEanble = ddgi.enable ? 1 : 0;
			}

			int32_t vxgiEnable = 0;
			int32_t lpvEnable = 0;

			int32_t enableIBL = skyboxData != nullptr ? 1 : 0;

			const glm::mat4* shadowTransforms = shadowData.shadowProjView;
			const glm::vec4* splitDepth = shadowData.splitDepth;
			const glm::mat4  lightView = shadowData.lightMatrix;
			const auto       numShadows = shadowData.shadowMapNum;
			//auto cubeMapMipLevels = envData->environmentMap ? envData->environmentMap->getMipMapLevels() - 1 : 0;
			int32_t renderMode = data.deferredOut;
			auto    cameraPos = glm::vec4{ cameraView.cameraTransform->getWorldPosition(), 1.f };

			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "lights", lights, sizeof(component::LightData) * numLights, false);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "shadowTransform", shadowTransforms);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "viewMatrix", &cameraView.view);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "lightView", &lightView);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "biasMat", &BIAS_MATRIX);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "viewProj", &cameraView.projView);

			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "cameraPosition", &cameraPos);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "splitDepths", splitDepth);

			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "shadowMapSize", &shadowData.shadowMapSize);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "initialBias", &shadowData.initialBias);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "lightCount", &numLights);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "shadowCount", &numShadows);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "mode", &renderMode);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "shadowMethod", &shadowData.shadowMethod);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableLPV", &lpvEnable);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableDDGI", &ddgiEanble);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableVXGI", &vxgiEnable);
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableIBL", &enableIBL);


			if (directionaLight != nullptr)
			{
				int32_t enableShadow = directionaLight->castShadow ? 1 : 0;
				data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableShadow", &enableShadow);
			}

			data.descriptorLightSet[0]->setTexture("uPreintegratedFG", data.preintegratedFG);
			data.descriptorLightSet[0]->setTexture("uShadowMap", shadowData.shadowTexture);

			if (!env.empty())
			{
				auto [entity, evnData] = *env.each().begin();
				data.descriptorLightSet[0]->setTexture("uPrefilterMap", evnData.prefilteredEnvironment == nullptr ? renderData.unitCube : evnData.prefilteredEnvironment);
				data.descriptorLightSet[0]->setTexture("uIrradianceSH", evnData.irradianceSH == nullptr ? renderData.unitTexture : evnData.irradianceSH);
				enableIBL = evnData.envLighting;
				data.descriptorLightSet[0]->setUniform("UniformBufferLight", "enableIBL", &enableIBL);

				if (evnData.prefilteredEnvironment != nullptr)
				{
					int32_t cubeMapMipLevels = evnData.prefilteredEnvironment->getMipMapLevels() - 1;
					data.descriptorLightSet[0]->setUniform("UniformBufferLight", "cubeMapMipLevels", &cubeMapMipLevels);
				}
			}
			else
			{
				data.descriptorLightSet[0]->setTexture("uPrefilterMap", renderData.unitCube);
				data.descriptorLightSet[0]->setTexture("uIrradianceSH", renderData.unitTexture);
			}

			int32_t ssaoEnable = 0;
			data.descriptorLightSet[0]->setUniform("UniformBufferLight", "ssaoEnable", &ssaoEnable);

			std::unordered_map<entt::entity, std::shared_ptr<glm::mat4[]>> boneTransform;

			auto forEachMesh = [&](const glm::mat4& worldTransform, std::shared_ptr<Mesh> mesh, bool hasStencil, component::SkinnedMeshRenderer* skinnedMesh, maple::Entity parent) {
				if (!mesh || !mesh->isActive())
					return;
				//culling
				auto bb = mesh->getBoundingBox()->transform(worldTransform);

				auto inside = cameraView.frustum.isInside(bb);

				if (inside)
				{
					PipelineInfo pipelineInfo{};
					pipelineInfo.polygonMode = PolygonMode::Fill;
					pipelineInfo.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
					pipelineInfo.clearTargets = false;
					pipelineInfo.swapChainTarget = false;
					pipelineInfo.pipelineName = "DeferredOffscreen";
					pipelineInfo.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::COLOR);
					pipelineInfo.colorTargets[1] = renderData.gbuffer->getBuffer(GBufferTextures::NORMALS);
					pipelineInfo.colorTargets[2] = renderData.gbuffer->getBuffer(GBufferTextures::PBR);
					pipelineInfo.colorTargets[3] = renderData.gbuffer->getBuffer(GBufferTextures::EMISSIVE);
					pipelineInfo.depthFunc = StencilType::Less;
					auto depthTest = data.depthTest;

					uint32_t start = 0;
					for (auto i = 0; i < mesh->getSubMeshIndex().size(); i++)
					{
						auto& cmd = data.commandQueue.emplace_back();
						cmd.mesh = mesh.get();
						cmd.transform = worldTransform;
						cmd.material = mesh->getSubMeshIndex().size() > mesh->getMaterial().size() ? data.defaultMaterial.get() : mesh->getMaterial()[i].get();
						cmd.material->bind(renderData.commandBuffer);
						cmd.start = start;
						cmd.count = mesh->getSubMeshIndex()[i] - start;

						pipelineInfo.shader = cmd.material->getShader();
						pipelineInfo.cullMode = cmd.material->isFlagOf(Material::RenderFlags::TwoSided) ? CullMode::None : CullMode::Back;
						pipelineInfo.transparencyEnabled = cmd.material->isFlagOf(Material::RenderFlags::AlphaBlend);
						if (depthTest && cmd.material->isFlagOf(Material::RenderFlags::DepthTest))
						{
							pipelineInfo.depthTarget = renderData.gbuffer->getDepthBuffer();
						}
						cmd.pipelineInfo = pipelineInfo;

						start = mesh->getSubMeshIndex()[i];
					}
				}
			};

			for (auto [entityHandle, mesh, trans] : meshQuery.each())
			{
				const auto& worldTransform = trans.getWorldMatrix();

				auto hasSDF = registry.getRegistry().any_of<sdf::component::MeshDistanceField>(entityHandle);

				forEachMesh(
					worldTransform,
					mesh.mesh,
					false,
					nullptr, {});
			}
		}

		inline auto onRender(deferred::global::component::DeferredData& data,const component::RendererData& renderData,ioc::Registry registry) -> void
		{
			data.descriptorColorSet[0]->update(renderData.commandBuffer);
			data.descriptorColorSet[2]->update(renderData.commandBuffer);

			std::shared_ptr<Pipeline> pipeline;
			bool                      shouldEnd = true;

			VertexBuffer::Ptr  lastVertexBuffer;
			IndexBuffer::Ptr   lastIndexBuffer;
			DescriptorSet::Ptr lastDescriptor;

			for (auto& command : data.commandQueue)
			{
				std::vector<std::shared_ptr<DescriptorSet>> descriptors;
				descriptors.resize(3);

				bool rebindDescriptor = false;

				auto currentPipeline = Pipeline::get(command.pipelineInfo);
				if (currentPipeline != pipeline)
				{
					pipeline = currentPipeline;
					rebindDescriptor = true;
					descriptors = data.descriptorColorSet;
				}

				auto materialDescriptor = command.material->getDescriptorSet(pipeline->getShader()->getName());
				bool bindDescriptor = false;
				if (materialDescriptor != lastDescriptor)
				{
					materialDescriptor->update(renderData.commandBuffer);
					lastDescriptor = materialDescriptor;
					bindDescriptor = true;
					descriptors[1] = materialDescriptor;
				}
				else
				{
					descriptors[1] = nullptr;
				}

				if (renderData.commandBuffer != nullptr && command.viewport == glm::ivec4{ -1 })
				{
					renderData.commandBuffer->bindPipeline(pipeline.get());
				}
				else
				{
					pipeline->bind(renderData.commandBuffer, command.viewport);
				}

				auto& pushConstants = pipeline->getShader()->getPushConstants()[0];

				auto id = command.mesh->getId();
				pushConstants.setValue("transform", &command.transform);
				pushConstants.setValue("meshId", &id);
				pipeline->getShader()->bindPushConstants(renderData.commandBuffer, pipeline.get());

				if (lastVertexBuffer != command.mesh->getVertexBuffer())
				{
					command.mesh->getVertexBuffer()->bind(renderData.commandBuffer, pipeline.get());
					command.mesh->getIndexBuffer()->bind(renderData.commandBuffer);
				}

				Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, descriptors);
				Renderer::drawIndexed(renderData.commandBuffer, DrawType::Triangle, command.count, command.start);

				if (lastVertexBuffer != command.mesh->getVertexBuffer())
				{
					lastVertexBuffer = command.mesh->getVertexBuffer();
					command.mesh->getVertexBuffer()->unbind();
					command.mesh->getIndexBuffer()->unbind();
				}

				if (command.viewport != glm::ivec4{ -1 })
				{
					pipeline->end(renderData.commandBuffer);
					shouldEnd = false;
				}
			}

			if (shouldEnd)
			{
				if (renderData.commandBuffer != nullptr)
				{
					renderData.commandBuffer->unbindPipeline();
				}
				else if (pipeline)
				{
					pipeline->end(renderData.commandBuffer);
				}
			}
		}

		auto registerGBufferRenderer(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<deferred::global::component::DeferredData>();
			builder->registerWithinQueue<deferred_offscreen::beginScene>(begin);
			builder->registerWithinQueue<deferred_offscreen::onRender>(renderer);
		}
	}        // namespace deferred_offscreen

	namespace deferred_lighting
	{
		inline auto onRender(
			deferred::global::component::DeferredData& data,
			capture_graph::component::RenderGraph& graph,
			const component::ShadowMapData& shadowData,
			const component::CameraView& cameraView,
			const component::RendererData& rendererData,
			const global::component::AppState& appState,
			ioc::Registry registry)
		{
			auto envQuery = registry.getRegistry().view<component::Environment>();
			auto raytraceShadowGroup = registry.getRegistry().view<raytraced_shadow::component::RaytracedShadow>();
			auto raytraceReflectionGroup = registry.getRegistry().view<raytraced_reflection::component::RaytracedReflection>();

			auto descriptorSet = data.descriptorLightSet[0];
			descriptorSet->setTexture("uColorSampler", rendererData.gbuffer->getBuffer(GBufferTextures::COLOR));
			descriptorSet->setTexture("uNormalSampler", rendererData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			descriptorSet->setTexture("uPBRSampler", rendererData.gbuffer->getBuffer(GBufferTextures::PBR));
			descriptorSet->setTexture("uDepthSampler", rendererData.gbuffer->getDepthBuffer());
			descriptorSet->setTexture("uIndirectLight", rendererData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING));
			descriptorSet->setTexture("uEmissiveSampler", rendererData.gbuffer->getBuffer(GBufferTextures::EMISSIVE));
			descriptorSet->setTexture("uShadowMap", shadowData.shadowTexture);

			if (!raytraceShadowGroup.empty() && shadowData.shadowMethod == ShadowingMethod::RaytracedShadow)
			{
				auto [ent,shadow] = *raytraceShadowGroup.each().begin();
				if (shadow.output)
				{
					descriptorSet->setTexture("uShadowMapRaytrace", shadow.output);
				}
			}
			else
			{
				descriptorSet->setTexture("uShadowMapRaytrace", rendererData.unitTexture);
			}

			if (!raytraceReflectionGroup.empty())
			{
				auto [entity, reflect] = *raytraceReflectionGroup.each().begin();
				if (reflect.output && appState.state == EditorState::Play && reflect.enable)
					descriptorSet->setTexture("uReflection", reflect.output);
				else
					descriptorSet->setTexture("uReflection", rendererData.unitTexture);
			}
			else
			{
				descriptorSet->setTexture("uReflection", rendererData.unitTexture);
			}

			if (!envQuery.empty())
			{
				auto [entity,envData] =*envQuery.each().begin();
				descriptorSet->setTexture("uIrradianceSH", envData.irradianceSH == nullptr ? rendererData.unitTexture : envData.irradianceSH);
				descriptorSet->setTexture("uPrefilterMap", envData.prefilteredEnvironment == nullptr ? rendererData.unitCube : envData.prefilteredEnvironment);
				descriptorSet->setTexture("uPreintegratedFG", data.preintegratedFG);
			}
			else
			{
				descriptorSet->setTexture("uIrradianceSH", rendererData.unitTexture );
				descriptorSet->setTexture("uPrefilterMap", rendererData.unitCube);
				descriptorSet->setTexture("uPreintegratedFG", data.preintegratedFG);
			}
			descriptorSet->update(rendererData.commandBuffer);

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName = "DeferredLighting";
			pipeInfo.shader = data.deferredLightShader;
			pipeInfo.polygonMode = PolygonMode::Fill;
			pipeInfo.cullMode = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = false;
			pipeInfo.colorTargets[0] = rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN);

			auto deferredLightPipeline = Pipeline::get(pipeInfo, data.descriptorLightSet, graph);
			deferredLightPipeline->bind(rendererData.commandBuffer);

			Renderer::bindDescriptorSets(deferredLightPipeline.get(), rendererData.commandBuffer, 0, data.descriptorLightSet);
			Renderer::drawMesh(rendererData.commandBuffer, deferredLightPipeline.get(), data.screenQuad.get());

			deferredLightPipeline->end(rendererData.commandBuffer);
		}

		auto registerDeferredLighting(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<deferred::global::component::DeferredData>();
			builder->registerWithinQueue<deferred_lighting::onRender>(renderer);
		}
	};        // namespace deferred_lighting
};            // namespace maple
