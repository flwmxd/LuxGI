//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "DDGIVisualization.h"

#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Raytrace/TracedData.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Raytrace/RaytraceConfig.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"
#include "Engine/Raytrace/RaytracedReflection.h"
#include "ImGui/ImGuiHelpers.h"

#include "GlobalDistanceField.h"
#include "GlobalSurfaceAtlas.h"

#include "RHI/BatchTask.h"
#include "RHI/DescriptorPool.h"
#include "RHI/DescriptorSet.h"
#include "RHI/GraphicsContext.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"
#include "RHI/VertexBuffer.h"

#include "Math/MathUtils.h"
#include "Others/Randomizer.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/Environment.h"
#include "Engine/Renderer/BindlessModule.h"

#include <imgui.h>
#include "ImGui/ImNotification.h"
#include <scope_guard.hpp>
namespace maple
{
	namespace
	{
		std::string lightTypeToString(maple::component::LightType type)
		{
			switch (type)
			{
			case maple::component::LightType::DirectionalLight:
				return "Directional Light";
			case maple::component::LightType::SpotLight:
				return "Spot Light";
			case maple::component::LightType::PointLight:
				return "Point Light";
			default:
				return "ERROR";
			}
		}

		int32_t stringToLightType(const std::string& type)
		{
			if (type == "Directional")
				return int32_t(maple::component::LightType::DirectionalLight);

			if (type == "Point")
				return int32_t(maple::component::LightType::PointLight);

			if (type == "Spot")
				return int32_t(maple::component::LightType::SpotLight);
			return 0;
		}
	}        // namespace
	namespace ddgi
	{
		namespace component
		{
			struct DDGIPipelineInternal
			{
				//raytrace pass.
				Texture2D::Ptr radiance;
				Texture2D::Ptr directionDepth;

				Texture2D::Ptr irradiance[2];
				Texture2D::Ptr depth[2];

				int32_t    frames = 0;
				int32_t    pingPong = 0;
				Randomizer rand;
			};

			struct RaytracePass
			{
				Shader::Ptr   shader;
				Pipeline::Ptr pipeline;

				Pipeline::Ptr sdfPipeline;

				DescriptorSet::Ptr samplerDescriptor;
				DescriptorSet::Ptr outpuDescriptor;

				DescriptorSet::Ptr sdfDescriptor;

				struct PushConstants
				{
					glm::mat4 randomOrientation;
					uint32_t  numFrames;
					uint32_t  infiniteBounces = 1;
					int32_t   numLights;
					float     intensity;
				} pushConsts;
				bool firstFrame = true;
			};

			struct ProbeUpdatePass
			{
				Shader::Ptr irradanceShader;
				Shader::Ptr depthShader;

				Pipeline::Ptr irradianceProbePipeline;
				Pipeline::Ptr depthProbePipeline;

				struct PushConsts
				{
					uint32_t firstFrame = 0;
				} pushConsts;

				std::vector<DescriptorSet::Ptr> irradianceDescriptors;
				std::vector<DescriptorSet::Ptr> depthDescriptors;
			};

			struct BorderUpdatePass
			{
				Shader::Ptr irradanceShader;
				Shader::Ptr depthShader;

				Pipeline::Ptr irradianceProbePipeline;
				Pipeline::Ptr depthProbePipeline;

				std::vector<DescriptorSet::Ptr> irradianceDescriptors;
				std::vector<DescriptorSet::Ptr> depthDescriptors;
			};

			struct SampleProbePass
			{
				Shader::Ptr shader;

				Pipeline::Ptr pipeline;

				std::vector<DescriptorSet::Ptr> descriptors;
			};
		}        // namespace component

		namespace init
		{
			inline auto initializeProbeGrid(
				ddgi::component::IrradianceVolume& pipeline,
				ddgi::component::DDGIPipelineInternal& internal,
				component::DDGIUniform& uniform, ioc::Registry world)
			{
				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
				MAPLE_ASSERT(totalProbes < std::numeric_limits<int16_t>::max(), "too many probes");
				{
					internal.radiance = Texture2D::create();
					internal.radiance->setName("DDGI Raytrace Radiance");
					internal.radiance->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);

					internal.directionDepth = Texture2D::create();
					internal.directionDepth->setName("DDGI Raytrace Direction Depth");
					internal.directionDepth->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);
				}

				{
					// 1-pixel of padding surrounding each probe, 1-pixel padding surrounding entire texture for alignment.
					const int32_t irradianceWidth = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t irradianceHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
					const int32_t depthWidth = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t depthHeight = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

					uniform.irradianceTextureWidth = irradianceWidth;
					uniform.irradianceTextureHeight = irradianceHeight;

					uniform.depthTextureWidth = depthWidth;
					uniform.depthTextureHeight = depthHeight;

					for (int32_t i = 0; i < 2; i++)
					{
						internal.irradiance[i] = Texture2D::create();
						internal.depth[i] = Texture2D::create();

						std::vector<uint8_t> data;
						data.resize(sizeof(uint8_t) * 2 * 2, 0);
						internal.depth[i]->setData(data.data());
						internal.depth[i]->buildTexture(TextureFormat::RG16F, depthWidth, depthHeight);
						internal.depth[i]->setName("DDGI Depth Probe Grid " + std::to_string(i));

						internal.irradiance[i]->buildTexture(TextureFormat::RGBA16, irradianceWidth, irradianceHeight);
						internal.irradiance[i]->setName("DDGI Irradiance Probe Grid " + std::to_string(i));

						data.resize(sizeof(uint8_t) * 2 * 4, 0);
						internal.irradiance[i]->setData(data.data());
					}
				}
			}
		}        // namespace init
	}            // namespace ddgi

	namespace trace_rays
	{
		inline auto system(ioc::Registry registry,
			component::RendererData& renderData,
			const global::component::RaytracingDescriptor& descriptor,
			const trace::global::component::RaytraceConfig& config,
			const sdf::surface::global::component::GlobalSurfacePublic& surfacePublic,
			const skybox_renderer::global::component::SkyboxData& skyboxData,
			const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic)
		{

			auto view = registry.getRegistry().view<
				ddgi::component::RaytracePass,
				ddgi::component::DDGIPipelineInternal,
				ddgi::component::IrradianceVolume,
				ddgi::component::DDGIUniform>();


			auto lights = registry.getRegistry().view<maple::component::Light>();

			for (auto [entity, raytracePass, internal, volume, uniform] : view.each())
			{
				auto pipeline = trace::isSoftTrace(config) ? raytracePass.sdfPipeline : raytracePass.pipeline;

				volume.currentIrrdance = internal.irradiance[1 - internal.pingPong];
				volume.currentDepth = internal.depth[1 - internal.pingPong];


				if (!trace::isSoftTrace(config))
				{
					raytracePass.samplerDescriptor->setTexture("uIrradiance", internal.irradiance[1 - internal.pingPong]);
					raytracePass.samplerDescriptor->setTexture("uDepth", internal.depth[1 - internal.pingPong]);
					raytracePass.samplerDescriptor->update(renderData.commandBuffer);
					raytracePass.outpuDescriptor->setTexture("iRadiance", internal.radiance);
					raytracePass.outpuDescriptor->setTexture("iDirectionDistance", internal.directionDepth);
					raytracePass.outpuDescriptor->update(renderData.commandBuffer, { {internal.radiance, internal.directionDepth},
																					ShaderType::RayGen,
																					ShaderType::RayGen,
																					AccessFlags::Read,
																					AccessFlags::Write });
				}
				else
				{
					raytracePass.sdfDescriptor->setTexture("iRadiance", internal.radiance);
					raytracePass.sdfDescriptor->setTexture("iDirectionDistance", internal.directionDepth);
				}

				raytracePass.pushConsts.numLights = std::distance(lights.begin(), lights.end());
				raytracePass.pushConsts.infiniteBounces = (volume.infiniteBounce && internal.frames != 0) ? 1 : 0;
				raytracePass.pushConsts.intensity = volume.intensity;
				raytracePass.pushConsts.numFrames = internal.frames;

				auto vec3 = glm::normalize(glm::vec3(internal.rand.nextReal(-1.f, 1.f),
					internal.rand.nextReal(-1.f, 1.f),
					internal.rand.nextReal(-1.f, 1.f)));

				raytracePass.pushConsts.randomOrientation = glm::mat4_cast(
					glm::angleAxis(internal.rand.nextReal(0.f, 1.f) * float(M_PI) * 2.0f, vec3));

				if (!trace::isSoftTrace(config))
				{
					raytracePass.pipeline->bind(renderData.commandBuffer);
					for (auto& push : pipeline->getShader()->getPushConstants())
					{
						push.setData(&raytracePass.pushConsts);
					}

					pipeline->getShader()->bindPushConstants(renderData.commandBuffer, pipeline.get());

					Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0,
						{ descriptor.sceneDescriptor,
						 descriptor.vboDescriptor,
						 descriptor.iboDescriptor,
						 descriptor.materialDescriptor,
						 descriptor.textureDescriptor,
						 raytracePass.samplerDescriptor,
						 raytracePass.outpuDescriptor });
					uint32_t probleCounts = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
					raytracePass.pipeline->traceRays(renderData.commandBuffer, uniform.raysPerProbe, probleCounts, 1);
					raytracePass.pipeline->end(renderData.commandBuffer);

					Renderer::imageBarrier(renderData.commandBuffer, { {internal.radiance, internal.directionDepth},
																	  ShaderType::RayGen,
																	  ShaderType::Compute,
																	  AccessFlags::Write,
																	  AccessFlags::Read });
				}
				else
				{
					raytracePass.sdfDescriptor->setTexture("uGlobalSDF", sdfPublic.texture);
					raytracePass.sdfDescriptor->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
					raytracePass.sdfDescriptor->setTexture("uSurfaceAtlasDepth", surfacePublic.surfaceDepth);
					raytracePass.sdfDescriptor->setTexture("uSurfaceAtlasTex", surfacePublic.surfaceLightCache);
					raytracePass.sdfDescriptor->setTexture("uSkybox", skyboxData.skybox == nullptr ? renderData.unitCube : skyboxData.skybox);

					raytracePass.sdfDescriptor->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);
					raytracePass.sdfDescriptor->setStorageBuffer("SDFObjectBuffer", surfacePublic.ssboObjectBuffer);
					raytracePass.sdfDescriptor->setStorageBuffer("SDFAtlasChunkBuffer", surfacePublic.chunkBuffer);
					raytracePass.sdfDescriptor->setStorageBuffer("SDFCullObjectBuffer", surfacePublic.culledObjectsBuffer);
					raytracePass.sdfDescriptor->setUniform("UniformBufferObject", "data", &sdfPublic.globalSurfaceAtlasData);
					raytracePass.sdfDescriptor->setUniform("UniformBufferObject", "sdfData", &sdfPublic.sdfCommonData);
					raytracePass.sdfDescriptor->setUniformBufferData("DDGIUBO", &uniform);

					uint32_t dispatchX = std::ceil(uniform.raysPerProbe / 16.f);
					uint32_t dispatchY = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
					Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1, pipeline.get(), &raytracePass.pushConsts,
						{ raytracePass.sdfDescriptor });

					Renderer::imageBarrier(renderData.commandBuffer, { {internal.radiance, internal.directionDepth},
																	  ShaderType::Compute,
																	  ShaderType::Compute,
																	  AccessFlags::Write,
																	  AccessFlags::Read });
				}
			}
		}
	}        // namespace trace_rays

	namespace end_frame
	{
		inline auto system(ioc::Registry registry)
		{
			for (auto [entity, internal] : registry.getRegistry().view<ddgi::component::DDGIPipelineInternal>().each())
			{
				internal.pingPong = 1 - internal.pingPong;
				internal.frames++;
			}
		}
	}        // namespace end_frame

	namespace probe_update
	{
		inline auto probeUpdate(
			ddgi::component::ProbeUpdatePass& pass,
			const ddgi::component::DDGIUniform& uniform,
			const component::RendererData& renderData,
			Pipeline::Ptr                                pipeline,
			std::vector<DescriptorSet::Ptr>& descriptors,
			const ddgi::component::DDGIPipelineInternal& internal,
			uint32_t                                     writeIndex)
		{
			descriptors[0]->update(renderData.commandBuffer);
			descriptors[1]->update(renderData.commandBuffer);
			descriptors[2]->update(renderData.commandBuffer);

			for (auto& push : pipeline->getShader()->getPushConstants())
			{
				pass.pushConsts.firstFrame = internal.frames == 0 ? 1 : 0;
				push.setData(&pass.pushConsts);
			}
			pipeline->bind(renderData.commandBuffer);
			pipeline->getShader()->bindPushConstants(renderData.commandBuffer, pipeline.get());

			const uint32_t dispatchX = static_cast<uint32_t>(uniform.probeCounts.x * uniform.probeCounts.y);
			const uint32_t dispatchY = static_cast<uint32_t>(uniform.probeCounts.z);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, descriptors);

			Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
			pipeline->end(renderData.commandBuffer);
		}

		inline auto system(ioc::Registry registry, component::RendererData& renderData)
		{

			for (auto [entity, probeUpdatePass, raytracePass, internal, pipeline, uniform] :
				registry.getRegistry().view<
				ddgi::component::ProbeUpdatePass,
				ddgi::component::RaytracePass,
				ddgi::component::DDGIPipelineInternal,
				ddgi::component::IrradianceVolume,
				ddgi::component::DDGIUniform
				>().each())
			{

				auto writeIdx = 1 - internal.pingPong;

				probeUpdatePass.irradianceDescriptors[0]->setTexture("uOutIrradiance", internal.irradiance[writeIdx]);
				probeUpdatePass.irradianceDescriptors[0]->setTexture("uOutDepth", internal.depth[writeIdx]);
				probeUpdatePass.irradianceDescriptors[1]->setTexture("uInputIrradiance", internal.irradiance[internal.pingPong]);
				probeUpdatePass.irradianceDescriptors[1]->setTexture("uInputDepth", internal.depth[internal.pingPong]);
				probeUpdatePass.irradianceDescriptors[1]->setUniform("DDGIUBO", "ddgi", &uniform);
				probeUpdatePass.irradianceDescriptors[2]->setTexture("uInputRadiance", internal.radiance);
				probeUpdatePass.irradianceDescriptors[2]->setTexture("uInputDirectionDepth", internal.directionDepth);

				probeUpdatePass.depthDescriptors[0]->setTexture("uOutIrradiance", internal.irradiance[writeIdx]);
				probeUpdatePass.depthDescriptors[0]->setTexture("uOutDepth", internal.depth[writeIdx]);
				probeUpdatePass.depthDescriptors[1]->setTexture("uInputIrradiance", internal.irradiance[internal.pingPong]);
				probeUpdatePass.depthDescriptors[1]->setTexture("uInputDepth", internal.depth[internal.pingPong]);
				probeUpdatePass.depthDescriptors[1]->setUniform("DDGIUBO", "ddgi", &uniform);
				probeUpdatePass.depthDescriptors[2]->setTexture("uInputRadiance", internal.radiance);
				probeUpdatePass.depthDescriptors[2]->setTexture("uInputDirectionDepth", internal.directionDepth);

				probeUpdate(probeUpdatePass, uniform, renderData, probeUpdatePass.irradianceProbePipeline, probeUpdatePass.irradianceDescriptors, internal, writeIdx);
				probeUpdate(probeUpdatePass, uniform, renderData, probeUpdatePass.depthProbePipeline, probeUpdatePass.depthDescriptors, internal, writeIdx);

				Renderer::memoryBarrier(renderData.commandBuffer, ShaderType::Compute, ShaderType::Compute, AccessFlags::Write, AccessFlags::ReadWrite);
			}
		}
	}        // namespace probe_update

	namespace border_update
	{
		inline auto borderUpdate(
			ddgi::component::BorderUpdatePass& pass,
			const ddgi::component::DDGIUniform& uniform,
			const component::RendererData& renderData,
			Pipeline::Ptr                       pipeline,
			std::vector<DescriptorSet::Ptr>& descriptors)
		{
			for (auto& descriptor : descriptors)
			{
				descriptor->update(renderData.commandBuffer);
			}

			pipeline->bind(renderData.commandBuffer);

			const uint32_t dispatchX = static_cast<uint32_t>(uniform.probeCounts.x * uniform.probeCounts.y);
			const uint32_t dispatchY = static_cast<uint32_t>(uniform.probeCounts.z);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, descriptors);

			Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
			pipeline->end(renderData.commandBuffer);
		}

		inline auto system(ioc::Registry registry, component::RendererData& renderData)
		{

			for (auto [entity, borderUpdatePass, raytracePass, internal, pipeline, uniform] :
				registry.getRegistry().view<
				ddgi::component::BorderUpdatePass,
				ddgi::component::RaytracePass,
				ddgi::component::DDGIPipelineInternal,
				ddgi::component::IrradianceVolume,
				ddgi::component::DDGIUniform
				>().each())
			{
				auto writeIdx = 1 - internal.pingPong;

				borderUpdatePass.irradianceDescriptors[0]->setTexture("iOutputIrradiance", internal.irradiance[writeIdx]);
				borderUpdatePass.irradianceDescriptors[0]->setTexture("iOutputDepth", internal.depth[writeIdx]);

				borderUpdatePass.depthDescriptors[0]->setTexture("iOutputIrradiance", internal.irradiance[writeIdx]);
				borderUpdatePass.depthDescriptors[0]->setTexture("iOutputDepth", internal.depth[writeIdx]);

				borderUpdate(borderUpdatePass, uniform, renderData, borderUpdatePass.irradianceProbePipeline, borderUpdatePass.irradianceDescriptors);
				borderUpdate(borderUpdatePass, uniform, renderData, borderUpdatePass.depthProbePipeline, borderUpdatePass.depthDescriptors);
			}
		}
	}        // namespace border_update

	namespace sample_probe
	{
		inline auto system(ioc::Registry registry, component::RendererData& renderData, const component::CameraView& cameraView)
		{
			for (auto [entity, pass, internal, uniform] :
				registry.getRegistry().view<
				ddgi::component::SampleProbePass,
				ddgi::component::DDGIPipelineInternal,
				ddgi::component::DDGIUniform
				>().each())
			{
				auto writeIdx = 1 - internal.pingPong;

				pass.descriptors[0]->setTexture("outColor", renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING));
				pass.descriptors[0]->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
				pass.descriptors[0]->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				pass.descriptors[0]->setTexture("uIrradiance", internal.irradiance[writeIdx]);
				pass.descriptors[0]->setTexture("uDepth", internal.depth[writeIdx]);
				pass.descriptors[0]->setUniform("DDGIUBO", "ddgi", &uniform);
				auto pos = glm::vec4(cameraView.cameraTransform->getWorldPosition(), 1.f);
				pass.descriptors[0]->setUniform("UniformBufferObject", "cameraPosition", &pos);
				pass.descriptors[0]->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));

				pass.descriptors[0]->update(renderData.commandBuffer, { {internal.irradiance[writeIdx], internal.depth[writeIdx]},
																	   ShaderType::Compute,
																	   ShaderType((uint32_t)ShaderType::Compute | (uint32_t)ShaderType::RayGen),
																	   AccessFlags::Write,
																	   AccessFlags::Read });
				pass.pipeline->bind(renderData.commandBuffer);
				const uint32_t dispatchX = static_cast<uint32_t>(std::ceil(float(renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING)->getWidth()) / 32.f));
				const uint32_t dispatchY = static_cast<uint32_t>(std::ceil(float(renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING)->getHeight()) / 32.f));
				Renderer::bindDescriptorSets(pass.pipeline.get(), renderData.commandBuffer, 0, pass.descriptors);
				Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
				pass.pipeline->end(renderData.commandBuffer);
			}
		}
	}        // namespace sample_probe

	namespace ddgi::base_update
	{
		inline auto system(ioc::Registry registry)
		{
			for (auto [entity, volume, uniform, transform, bbox, internal] :
				registry.getRegistry().view<
				ddgi::component::IrradianceVolume,
				ddgi::component::DDGIUniform,
				maple::component::Transform,
				maple::component::BoundingBoxComponent,
				ddgi::component::DDGIPipelineInternal
				>().each())
			{
				auto aabb = bbox.box.transform(transform.getWorldMatrix());

				if (glm::vec3(uniform.startPosition) != aabb.min)
				{
					glm::vec3 sceneLength = aabb.max - aabb.min;

					uniform.startPosition = glm::vec4(aabb.min, 1.f);
					uniform.step = glm::vec4(volume.probeDistance);
					uniform.probeCounts = glm::ivec4(glm::ivec3(sceneLength / volume.probeDistance) + glm::ivec3(2), 1.f);
					LOGI("ProbeCounts : {},{},{}", uniform.probeCounts.x, uniform.probeCounts.y, uniform.probeCounts.z);

					const int32_t irradianceWidth = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t irradianceHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
					const int32_t depthWidth = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t depthHeight = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

					uniform.irradianceTextureWidth = irradianceWidth;
					uniform.irradianceTextureHeight = irradianceHeight;
					uniform.depthTextureWidth = depthWidth;
					uniform.depthTextureHeight = depthHeight;

					for (int32_t i = 0; i < 2; i++)
					{
						if (internal.depth[i]->getWidth() != depthWidth || internal.depth[i]->getWidth() != depthHeight)
						{
							internal.depth[i]->buildTexture(TextureFormat::RG16F, depthWidth, depthHeight);
						}

						if (internal.irradiance[i]->getWidth() != irradianceWidth || internal.irradiance[i]->getHeight() != irradianceHeight)
						{
							internal.irradiance[i]->buildTexture(TextureFormat::RG16F, irradianceWidth, irradianceHeight);
						}
					}

					uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
					if (totalProbes != internal.radiance->getHeight() || volume.raysPerProbe != internal.radiance->getWidth())
					{
						internal.radiance->buildTexture(TextureFormat::RGBA16, volume.raysPerProbe, totalProbes);
						internal.directionDepth->buildTexture(TextureFormat::RGBA16, volume.raysPerProbe, totalProbes);
					}
				}
			}
		}
	}        // namespace ddgi::base_update

	namespace ddgi::on_game_start
	{
		inline auto system(ioc::Registry registry,
			const maple::component::WindowSize& windowSize,
			const global::component::GraphicsContext& context)
		{
			for (auto [entity, volume, bbox, uniform, transform] :
				registry.getRegistry().view<
				ddgi::component::IrradianceVolume,
				maple::component::BoundingBoxComponent,
				ddgi::component::DDGIUniform,
				maple::component::Transform
				>().each())
			{
				auto& pipe = registry.addComponent<ddgi::component::DDGIPipelineInternal>(entity);
				auto& probeUpdatePass = registry.addComponent<ddgi::component::ProbeUpdatePass>(entity);
				auto& borderUpdatePass = registry.addComponent<ddgi::component::BorderUpdatePass>(entity);
				auto& sampleProbePass = registry.addComponent<ddgi::component::SampleProbePass>(entity);

				float scaleDivisor = powf(2.0f, float(volume.scale));
				volume.width = windowSize.width / scaleDivisor;
				volume.height = windowSize.height / scaleDivisor;

				auto& raytracePass = registry.addComponent<ddgi::component::RaytracePass>(entity);

				if (context.context->isRaytracingSupported())
				{
					raytracePass.shader = Shader::create("shaders/DDGI/GIRays.shader", { {"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT} });
					PipelineInfo info;
					info.pipelineName = "GIRays";
					info.shader = raytracePass.shader;
					info.maxRayRecursionDepth = 1;
					raytracePass.pipeline = Pipeline::get(info);

					raytracePass.samplerDescriptor = DescriptorSet::create({ 5, raytracePass.shader.get() });
					raytracePass.outpuDescriptor = DescriptorSet::create({ 6, raytracePass.shader.get() });
				}

				{
					PipelineInfo info;
					probeUpdatePass.irradanceShader = Shader::create("shaders/DDGI/IrradianceProbeUpdate.shader");
					info.pipelineName = "IrradianceProbeUpdatePipeline";
					info.shader = probeUpdatePass.irradanceShader;
					probeUpdatePass.irradianceProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					probeUpdatePass.depthShader = Shader::create("shaders/DDGI/DepthProbeUpdate.shader");
					info.pipelineName = "DepthProbeUpdatePipeline";
					info.shader = probeUpdatePass.depthShader;
					probeUpdatePass.depthProbePipeline = Pipeline::get(info);
				}

				for (uint32_t i = 0; i < 3; i++)
				{
					probeUpdatePass.irradianceDescriptors.emplace_back(DescriptorSet::create({ i, probeUpdatePass.irradanceShader.get() }));
					probeUpdatePass.depthDescriptors.emplace_back(DescriptorSet::create({ i, probeUpdatePass.depthShader.get() }));
				}

				{
					PipelineInfo info;
					borderUpdatePass.irradanceShader = Shader::create("shaders/DDGI/IrradianceBorderUpdate.shader");
					info.pipelineName = "IrradianceBorderUpdatePipeline";
					info.shader = borderUpdatePass.irradanceShader;
					borderUpdatePass.irradianceProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					borderUpdatePass.depthShader = Shader::create("shaders/DDGI/DepthBorderUpdate.shader");
					info.pipelineName = "DepthBorderUpdatePipeline";
					info.shader = borderUpdatePass.depthShader;
					borderUpdatePass.depthProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					sampleProbePass.shader = Shader::create("shaders/DDGI/SampleProbe.shader");
					info.pipelineName = "SampleProbePipeline";
					info.shader = sampleProbePass.shader;
					sampleProbePass.pipeline = Pipeline::get(info);
					sampleProbePass.descriptors.emplace_back(DescriptorSet::create({ 0, sampleProbePass.shader.get() }));
				}

				borderUpdatePass.irradianceDescriptors.emplace_back(DescriptorSet::create({ 0, borderUpdatePass.irradanceShader.get() }));
				borderUpdatePass.depthDescriptors.emplace_back(DescriptorSet::create({ 0, borderUpdatePass.depthShader.get() }));

				PipelineInfo info;

				{
					info.pipelineName = "SDFGIRays";
					info.shader = Shader::create("shaders/DDGI/GISDFRays.shader");
					raytracePass.sdfPipeline = Pipeline::get(info);
					raytracePass.sdfDescriptor = DescriptorSet::create({ 0, info.shader.get() });
				}

				volume.ddgiCommon = raytracePass.samplerDescriptor;

				{
					auto      aabb = bbox.box.transform(transform.getWorldMatrix());
					glm::vec3 sceneLength = aabb.max - aabb.min;
					// Add 2 more probes to fully cover scene.
					uniform.probeCounts = glm::ivec4(glm::ivec3(sceneLength / volume.probeDistance) + glm::ivec3(2), 1);
					LOGI("SceneLength : {},{},{}", sceneLength.x, sceneLength.y, sceneLength.z);
					uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
					uniform.startPosition = glm::vec4(aabb.min, 1.f);
					uniform.step = glm::vec4(volume.probeDistance);
					uniform.maxDistance = volume.probeDistance * 1.5f;
					uniform.depthSharpness = volume.depthSharpness;
					uniform.hysteresis = volume.hysteresis;
					uniform.normalBias = volume.normalBias;
					uniform.ddgiGamma = volume.ddgiGamma;
					uniform.irradianceTextureWidth = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					uniform.irradianceTextureHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
					uniform.depthTextureWidth = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					uniform.depthTextureHeight = (DepthOctSize + 2) * uniform.probeCounts.z + 2;
					if(raytracePass.samplerDescriptor)
						raytracePass.samplerDescriptor->setUniform("DDGIUBO", "ddgi", &uniform);
				}

				ddgi::init::initializeProbeGrid(volume, pipe, uniform, registry);
			}
		}
	}        // namespace ddgi::on_game_start

	namespace ddgi::on_game_end
	{
		inline auto system(ioc::Registry registry,
			maple::component::RendererData& renderData)
		{

			for (auto [entity, volume, _1, _2] :
				registry.getRegistry().view<ddgi::component::IrradianceVolume,
				maple::component::BoundingBoxComponent,
				ddgi::component::DDGIUniform>().each())
			{
				volume.currentDepth = nullptr;
				volume.currentIrrdance = nullptr;
				volume.ddgiCommon = nullptr;

				registry.removeComponent<ddgi::component::DDGIPipelineInternal>(entity);
				registry.removeComponent<ddgi::component::ProbeUpdatePass>(entity);
				registry.removeComponent<ddgi::component::BorderUpdatePass>(entity);
				registry.removeComponent<ddgi::component::SampleProbePass>(entity);
				registry.removeComponent<ddgi::component::RaytracePass>(entity);
			}
		}
	}        // namespace ddgi::on_game_end

	namespace on_imgui
	{
		inline auto system(ioc::Registry registry,
			trace::global::component::RaytraceConfig& config,
			global::component::SceneTransformChanged& changed,
			const maple::component::WindowSize& windowSize,
			const global::component::GraphicsContext& context)
		{
			if (ImGui::Begin("DDGI Surface Debugger."))
			{
				if (ImGui::Button("Run DDGI"))
				{
					ddgi::on_game_start::system(registry, windowSize, context);
					ImNotification::makeNotification("DDGI", "DDGI is Running", ImNotification::Type::Success, 10000);
				}
				
				
				ImGui::Separator();
				ImGui::Columns(2);

				for (auto [entity, reflection] :
					registry.getRegistry().view<raytraced_reflection::component::RaytracedReflection>().each())
				{
					ImGuiHelper::property("Reflection Enable", reflection.enable);
					ImGuiHelper::property("Reflection Denoise Enable", reflection.denoise);
				}
				ImGui::Separator();
				
				if (auto id = ImGuiHelper::combox("Trace Type", trace::Names, trace::Length, config.traceType); id != -1)
				{
					if (id == trace::Id::RayTrace)
					{
						if (context.context->isRaytracingSupported())
						{
							config.traceType = static_cast<trace::Id>(id);
							changed.dirty = true;
							ImNotification::makeNotification("Tips", "Raytracing now", ImNotification::Type::Success);
						}
						else
						{
							ImNotification::makeNotification("Tips", "Raytracing not supported...", ImNotification::Type::Error);
						}
					}
					else
					{
						config.traceType = static_cast<trace::Id>(id);
						ImNotification::makeNotification("Tips", "SDF-Tracing now", ImNotification::Type::Success);

					}
				}
				ImGui::Columns(1);
				ImGui::Separator();

				if (ImGui::CollapsingHeader("Objects...."))
				{
					for (auto [entity, transform, name] : registry.getRegistry().view<maple::component::Transform, maple::component::NameComponent>().each()) {

						auto hierarchy = registry.getRegistry().try_get<maple::component::Hierarchy>(entity);
						if (hierarchy == nullptr || hierarchy->parent == entt::null)
						{
							if (ImGui::CollapsingHeader(name.name.c_str()))
							{
								auto rotation = glm::degrees(transform.getLocalOrientation());
								auto position = transform.getLocalPosition();
								auto scale = transform.getLocalScale();

								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
								ImGui::Columns(2);
								ImGui::Separator();

								//##################################################
								if (ImGuiHelper::property("Position", position, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
								{
									transform.setLocalPosition(position);
								}
								//##################################################
								if (ImGuiHelper::property("Rotation", rotation, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.5))
								{
									transform.setLocalOrientation(glm::radians(rotation));
								}
								//##################################################
								if (ImGuiHelper::property("Scale", scale, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
								{
									transform.setLocalScale(scale);
								}
								//##################################################

								ImGui::Columns(1);
								ImGui::Separator();
								ImGui::PopStyleVar();

								if (registry.getRegistry().any_of<maple::component::Light>(entity))
								{
									if (!registry.getRegistry().any_of<maple::component::Environment>(entity)) 
									{
										if (ImGui::Button("Add Skybox"))
										{
											registry.getRegistry().get_or_emplace<maple::component::Environment>(entity).pseudoSky = true;
										}
									}
									
									auto& light = registry.getRegistry().get<maple::component::Light>(entity);

									ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
									ImGui::Columns(2);
									ImGui::Separator();

									if (static_cast<component::LightType>(light.lightData.type) != component::LightType::DirectionalLight)
									{
										ImGuiHelper::property("Radius", light.lightData.radius, 0.0f, 100.0f, ImGuiHelper::PropertyFlag::DragFloat);
										ImGuiHelper::property("Radius For soft shadow", light.lightData.direction.w, 0.0f, 10.0f);
									}
									else
									{
										ImGuiHelper::property("Radius For soft shadow", light.lightData.direction.w, 0.0f, 1.0f);
									}

									ImGuiHelper::property("Color", light.lightData.color, true, ImGuiHelper::PropertyFlag::ColorProperty);

									if (ImGuiHelper::property("Intensity", light.lightData.intensity, 0.0f, 100.0f))
									{        //update once
										transform.setLocalPosition(transform.getLocalPosition());
									}

									if (static_cast<component::LightType>(light.lightData.type) == component::LightType::SpotLight)
										ImGuiHelper::property("Angle", light.lightData.angle, 0.0f, 1.0f);

									if (ImGuiHelper::property("Cast Shadow", light.castShadow))
									{
										transform.setLocalPosition(transform.getLocalPosition());
									}

									ImGui::AlignTextToFramePadding();
									ImGui::TextUnformatted("Light Type");
									ImGui::NextColumn();
									ImGui::PushItemWidth(-1);

									const char* lights[] = { "Directional Light", "Spot Light", "Point Light" };
									auto        currLight = lightTypeToString(component::LightType(int32_t(light.lightData.type)));

									if (ImGui::BeginCombo("LightType", currLight.c_str(), 0))
									{
										for (auto n = 0; n < 3; n++)
										{
											if (ImGui::Selectable(lights[n]))
											{
												light.lightData.type = n;
											}
										}
										ImGui::EndCombo();
									}

									ImGui::Columns(1);
									ImGui::Separator();
									ImGui::PopStyleVar();
								}
							}
						}
					}
				}

				ImGui::Separator();
			


				for (auto [entity, raytracePass, internal, volume, uniform, visual, transform] :
					registry.getRegistry().view<
					ddgi::component::RaytracePass,
					ddgi::component::DDGIPipelineInternal,
					ddgi::component::IrradianceVolume,
					ddgi::component::DDGIUniform,
					ddgi::component::DDGIVisualization,
					maple::component::Transform
					>().each())
				{
				
					ImGui::Columns(1);

					if (ImGui::CollapsingHeader("Radiance") && internal.radiance)
					{
						ImGuiHelper::image(internal.radiance.get(), { internal.radiance->getWidth(), internal.radiance->getHeight() });
					}

					if (ImGui::CollapsingHeader("Direction-Depth") && internal.directionDepth)
					{
						ImGuiHelper::image(internal.directionDepth.get(), { internal.directionDepth->getWidth(), internal.directionDepth->getHeight() });
					}

					if (ImGui::CollapsingHeader("Irradiance") && internal.irradiance[0])
					{
						ImGuiHelper::image(internal.irradiance[0].get(), { internal.irradiance[0]->getWidth(), internal.irradiance[0]->getHeight() });
					}

					if (ImGui::CollapsingHeader("Depth") && internal.depth[0])
					{
						ImGuiHelper::image(internal.depth[0].get(), { internal.depth[0]->getWidth(), internal.depth[0]->getHeight() });
					}

					if (ImGui::CollapsingHeader("DDGI Volume"))
					{
						auto rotation = glm::degrees(transform.getLocalOrientation());
						auto position = transform.getLocalPosition();
						auto scale = transform.getLocalScale();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
						ImGui::Columns(2);
						ImGui::Separator();

						//##################################################
						if (ImGuiHelper::property("Position", position, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
						{
							transform.setLocalPosition(position);
						}
						//##################################################
						if (ImGuiHelper::property("Rotation", rotation, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.5))
						{
							transform.setLocalOrientation(glm::radians(rotation));
						}
						//##################################################
						if (ImGuiHelper::property("Scale", scale, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
						{
							transform.setLocalScale(scale);
						}
						//##################################################

						ImGui::Columns(1);
						ImGui::Separator();
						ImGui::PopStyleVar();

						//##################################################
						ImGui::Columns(2);
						ImGui::Separator();

						bool updated = false;

						ImGuiHelper::property("Probe Debugger", visual.enable);

						ImGuiHelper::property("Enable", volume.enable);

						updated = ImGuiHelper::property("Infinite Bounce", volume.infiniteBounce) || updated;
						updated = ImGuiHelper::property("Hysteresis", volume.hysteresis, 0.0f, 1.f) || updated;
						updated = ImGuiHelper::property("Normal Bias", volume.normalBias, 0.f, 5.f) || updated;
						updated = ImGuiHelper::property("Depth Sharpness", volume.depthSharpness, 0.f, 100.f) || updated;

						if (updated)
							registry.getRegistry().patch<ddgi::component::IrradianceVolume>(entity);

						ImGui::Separator();
						ImGui::Columns(1);
					}
				}

			}
			ImGui::End();
		}
	}        // namespace on_imgui

	namespace ddgi
	{
		namespace delegates
		{
			inline auto uniformChanged(
				ddgi::component::IrradianceVolume& pipeline, 
				component::DDGIUniform& uniform,
				maple::component::BoundingBoxComponent& bBox,
				const maple::component::Transform& transform,
				ddgi::component::RaytracePass* pass)
			{

				auto newBb = bBox.box.transform(transform.getWorldMatrix());

				glm::vec3 sceneLength = newBb.max - newBb.min;
				// Add 2 more probes to fully cover scene.
				uniform.probeCounts = glm::ivec4(glm::ivec3(sceneLength / pipeline.probeDistance) + glm::ivec3(2), 1);
				LOGI("ProbeCounts : {},{},{}", uniform.probeCounts.x, uniform.probeCounts.y, uniform.probeCounts.z);

				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

				uniform.startPosition = glm::vec4(bBox.box.min, 1.f);
				uniform.step = glm::vec4(pipeline.probeDistance);

				uniform.maxDistance = pipeline.probeDistance * 1.5f;
				uniform.depthSharpness = pipeline.depthSharpness;
				uniform.hysteresis = pipeline.hysteresis;
				uniform.normalBias = pipeline.normalBias;

				uniform.ddgiGamma = pipeline.ddgiGamma;
				uniform.irradianceTextureWidth = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.irradianceTextureHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
				uniform.depthTextureWidth = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.depthTextureHeight = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

				if (pass)
				{
					if (pass->samplerDescriptor)
						pass->samplerDescriptor->setUniform("DDGIUBO", "ddgi", &uniform);
				}
			}
		}        // namespace delegates

		auto registerDDGI(SystemQueue& begin, SystemQueue& render, std::shared_ptr<SystemBuilder> builder) -> void
		{
			//if (builder->getGlobalComponent<global::component::GraphicsContext>().context->isRaytracingSupported())
			{
				builder->addDependency<ddgi::component::IrradianceVolume, maple::component::Transform>();
				builder->addDependency<ddgi::component::IrradianceVolume, maple::component::BoundingBoxComponent>();
				builder->addDependency<ddgi::component::IrradianceVolume, ddgi::component::DDGIUniform>();
				builder->addDependency<ddgi::component::IrradianceVolume, ddgi::component::DDGIVisualization>();

				builder->onUpdate<ddgi::component::IrradianceVolume, delegates::uniformChanged>();

				builder->registerWithinQueue<base_update::system>(begin);
				builder->registerGameEnded<on_game_end::system>();

				builder->registerWithinQueue<trace_rays::system>(render);
				builder->registerWithinQueue<probe_update::system>(render);
				builder->registerWithinQueue<border_update::system>(render);
				builder->registerWithinQueue<sample_probe::system>(render);
				builder->registerWithinQueue<end_frame::system>(render);
				builder->registerOnImGui<on_imgui::system>();
			}
		}
	}        // namespace ddgi
}        // namespace maple