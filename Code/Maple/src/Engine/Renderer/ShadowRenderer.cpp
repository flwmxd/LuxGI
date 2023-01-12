//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "ShadowRenderer.h"
#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Scene.h"

#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Core.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/JobSystem.h"

#include "IoC/Registry.h"

namespace maple
{
	namespace        //private block
	{
		inline auto updateCascades(const component::CameraView& camera, component::ShadowMapData& shadowData, component::Light* light)
		{
			PROFILE_FUNCTION();

			float cascadeSplits[SHADOWMAP_MAX];

			const float nearClip = camera.nearPlane;
			const float farClip = camera.farPlane;
			const float clipRange = farClip - nearClip;
			const float minZ = nearClip;
			const float maxZ = nearClip + clipRange;
			const float range = maxZ - minZ;
			const float ratio = maxZ / minZ;
			// Calculate split depths based on view camera frustum
			// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
			for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
			{
				float p = static_cast<float>(i + 1) / static_cast<float>(shadowData.shadowMapNum);
				float log = minZ * std::pow(ratio, p);
				float uniform = minZ + range * p;
				float d = shadowData.cascadeSplitLambda * (log - uniform) + uniform;
				cascadeSplits[i] = (d - nearClip) / clipRange;
			}

			float lastSplitDist = 0.0;
			for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
			{
				PROFILE_SCOPE("Create Cascade");
				float splitDist = cascadeSplits[i];

				auto frum = camera.frustum;

				glm::vec3* frustumCorners = frum.getVertices();

				for (uint32_t i = 0; i < 4; i++)
				{
					glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
					frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
					frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
				}

				glm::vec3 frustumCenter = glm::vec3(0.0f);
				for (uint32_t i = 0; i < 8; i++)
				{
					frustumCenter += frustumCorners[i];
				}
				frustumCenter /= 8.0f;

				float radius = 0.0f;
				for (uint32_t i = 0; i < 8; i++)
				{
					float distance = glm::length(frustumCorners[i] - frustumCenter);
					radius = glm::max(radius, distance);
				}
				radius = std::ceil(radius * 16.0f) / 16.0f;

				glm::vec3 maxExtents = glm::vec3(radius);
				glm::vec3 minExtents = -maxExtents;

				glm::vec3 lightDir = glm::normalize(glm::vec3(light->lightData.direction));
				glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, maple::UP);
				glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -(maxExtents.z - minExtents.z), maxExtents.z - minExtents.z);

				shadowData.splitDepth[i] = glm::vec4(camera.nearPlane + splitDist * clipRange) * -1.f;
				shadowData.shadowProjView[i] = lightOrthoMatrix * lightViewMatrix;
				if (i == 0)
				{
					shadowData.lightMatrix = lightViewMatrix;
					shadowData.lightDir = lightDir;
				}

				lastSplitDist = cascadeSplits[i];
			}
		}
	}        // namespace

	namespace shadow_map_pass
	{
		auto beginScene(component::ShadowMapData& shadowData,
			const component::CameraView& cameraView,
			const global::component::SceneTransformChanged& sceneChanged,
			ioc::Registry registry)
		{
			auto lightQuery = registry.getRegistry().view<component::Light>();
			auto meshQuery = registry.getRegistry().view<component::MeshRenderer, component::Transform>();

			if (sceneChanged.dirty || shadowData.dirty)
			{
				shadowData.dirty = false;
				for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
				{
					shadowData.cascadeCommandQueue[i].clear();
				}

				if (!lightQuery.empty())
				{
					component::Light* directionaLight = nullptr;

					for (auto [entity, light] : lightQuery.each())
					{
						if (static_cast<component::LightType>(light.lightData.type) == component::LightType::DirectionalLight)
						{
							directionaLight = &light;
							break;
						}
					}

					if (directionaLight && directionaLight->castShadow)
					{
						if (directionaLight)
						{
							updateCascades(cameraView, shadowData, directionaLight);

							for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
							{
								shadowData.cascadeFrustums[i].from(shadowData.shadowProjView[i]);
							}
						}

						JobSystem::Context context;
						JobSystem::dispatch(context, shadowData.shadowMapNum, 1, [&](JobSystem::JobDispatchArgs args) {
							for (auto [entity, mesh, trans] : meshQuery.each())
							{
								if (mesh.castShadow && mesh.active && mesh.mesh != nullptr)
								{
									auto bb = mesh.mesh->getBoundingBox()->transform(trans.getWorldMatrix());
									auto inside = shadowData.cascadeFrustums[args.jobIndex].isInside(bb);
									if (inside)
									{
										auto& cmd = shadowData.cascadeCommandQueue[args.jobIndex].emplace_back();
										cmd.mesh = mesh.mesh.get();
										cmd.transform = trans.getWorldMatrix();
									}
								}
							}});
						JobSystem::wait(context);

						shadowData.descriptorSet[0]->setUniform("UniformBufferObject", "projView", shadowData.shadowProjView);
					}
				}
			}
		}

		inline auto onRender(component::ShadowMapData& shadowData,
			const component::RendererData& rendererData,
			capture_graph::component::RenderGraph& renderGraph,
			const global::component::SceneTransformChanged& sceneChanged,
			ioc::Registry registry
			)
		{

			if (sceneChanged.dirty)
			{
				shadowData.descriptorSet[0]->update(rendererData.commandBuffer);

				auto pipeline = shadowData.pipeline;

				for (uint32_t i = 0; i < shadowData.shadowMapNum; ++i)
				{
					//GPUProfile("Shadow Layer Pass");
					pipeline->bind(rendererData.commandBuffer, i);

					for (auto& command : shadowData.cascadeCommandQueue[i])
					{
						Mesh* mesh = command.mesh;
						const auto& trans = command.transform;
						auto& pushConstants = shadowData.shader->getPushConstants()[0];

						pushConstants.setValue("transform", (void*)&trans);
						pushConstants.setValue("cascadeIndex", (void*)&i);

						shadowData.shader->bindPushConstants(rendererData.commandBuffer, pipeline.get());

						Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, shadowData.descriptorSet);
						Renderer::drawMesh(rendererData.commandBuffer, pipeline.get(), mesh);
					}
					pipeline->end(rendererData.commandBuffer);
				}
			}
		}
	}        // namespace shadow_map_pass

	namespace shadow_map
	{
		auto registerShadowMap(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerGlobalComponent<component::ShadowMapData, component::RendererData>([](component::ShadowMapData& data,
				component::RendererData& renderData) {
					data.shadowTexture = TextureDepthArray::create(SHADOWMAP_SiZE_MAX, SHADOWMAP_SiZE_MAX, data.shadowMapNum, renderData.commandBuffer);
					data.shader = Shader::create("shaders/Shadow.shader");

					data.descriptorSet.resize(1);

					data.descriptorSet[0] = DescriptorSet::create({ 0, data.shader.get() });

					data.cascadeCommandQueue[0].reserve(500);
					data.cascadeCommandQueue[1].reserve(500);
					data.cascadeCommandQueue[2].reserve(500);
					data.cascadeCommandQueue[3].reserve(500);

					data.shadowTexture->setName("uShaderMapSampler");

					PipelineInfo pipelineInfo{};

					pipelineInfo.shader = data.shader;
					pipelineInfo.cullMode = CullMode::Back;
					pipelineInfo.transparencyEnabled = false;
					pipelineInfo.depthBiasEnabled = false;
					pipelineInfo.depthArrayTarget = data.shadowTexture;
					pipelineInfo.clearTargets = true;
					pipelineInfo.depthTest = true;
					pipelineInfo.clearColor = { 0, 0, 0, 0 };
					pipelineInfo.pipelineName = "ShadowMapping-";

					data.pipeline = Pipeline::get(pipelineInfo);
				});

			builder->registerWithinQueue<shadow_map_pass::beginScene>(begin);
			builder->registerWithinQueue<shadow_map_pass::onRender>(renderer);
		}
	};        // namespace shadow_map
}        // namespace maple
