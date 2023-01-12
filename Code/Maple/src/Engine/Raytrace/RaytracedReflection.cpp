//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RaytracedReflection.h"
#include "RaytraceConfig.h"

#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/GBuffer.h"
#include "Engine/Noise/BlueNoise.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "Scene/Component/AppState.h"
#include "Scene/Component/Transform.h"
#include "Engine/Renderer/BindlessModule.h"

#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/DDGI/GlobalDistanceField.h"
#include "Engine/DDGI/GlobalSurfaceAtlas.h"

#include "RHI/AccelerationStructure.h"
#include "RHI/DescriptorPool.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"

#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	namespace
	{
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_X = 8;
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_Y = 8;
	}        // namespace

	namespace raytraced_reflection
	{
		namespace component
		{
			struct ReflectionPipeline
			{
				Shader::Ptr   traceShader;
				Pipeline::Ptr tracePipeline;

				DescriptorSet::Ptr gbufferSet;        //6 - GBuffer
				DescriptorSet::Ptr noiseSet;          //7 - Noise
				DescriptorSet::Ptr outSet;            //8 - Out...

				DescriptorSet::Ptr sdfSet;        //0 - Out...
				Shader::Ptr        sdfShader;

				Texture2D::Ptr outColor;

				struct PushConstants
				{
					float     bias = 0.5f;
					float     trim = 0.8f;
					float     intensity = 0.5f;
					float     roughDDGIIntensity = 0.5f;
					uint32_t  numLights = 0;
					uint32_t  numFrames = 0;
					uint32_t  sampleGI = 1;
					uint32_t  approximateWithDDGI = 1;
					glm::vec4 cameraPosition;
					glm::mat4 viewProjInv;
					glm::mat4 viewProj;
					glm::mat4 view;
					glm::mat4 projInv;
				} pushConsts;

				int32_t pingPong = 0;

				//Temporal - Accumulation
				Texture2D::Ptr outputs[2];
				Texture2D::Ptr currentMoments[2];
				bool           firstFrame = true;
			};

			struct TemporalAccumulator
			{
				StorageBuffer::Ptr denoiseDispatchBuffer;
				StorageBuffer::Ptr denoiseTileCoordsBuffer;        // indirect?

				StorageBuffer::Ptr copyTileCoordsBuffer;
				StorageBuffer::Ptr copyDispatchBuffer;        // indirect?

				Shader::Ptr   resetArgsShader;
				Pipeline::Ptr resetPipeline;

				DescriptorSet::Ptr indirectDescriptorSet;

				/*
				* 0 -> Moment/History
				* 1 -> GBuffer
				* 2 -> PrevGBuffer
				* 3 -> Args
				*/
				std::vector<DescriptorSet::Ptr> descriptorSets;
				Shader::Ptr                     reprojectionShader;
				Pipeline::Ptr                   reprojectionPipeline;

				struct PushConstants
				{
					glm::vec3 cameraDelta = {};
					float     alpha = 0.01f;
					float     momentsAlpha = 0.2f;
					int32_t   approximateWithDDGI = 1;
				} pushConsts;
			};

			struct AtrousFiler
			{
				struct PushConstants
				{
					int32_t radius = 1;
					int32_t stepSize = 0;
					float   phiColor = 10.0f;
					float   phiNormal = 32.f;
					float   sigmaDepth = 1.f;
					int32_t approximateWithDDGI = 1;
				} pushConsts;

				int32_t iterations = 4;

				Shader::Ptr   atrousFilerShader;
				Pipeline::Ptr atrousFilerPipeline;

				Shader::Ptr   copyTilesShader;
				Pipeline::Ptr copyTilesPipeline;

				DescriptorSet::Ptr copyWriteDescriptorSet[2];
				DescriptorSet::Ptr copyReadDescriptorSet[2];

				DescriptorSet::Ptr gBufferSet;
				DescriptorSet::Ptr inputSet;
				DescriptorSet::Ptr argsSet;

				Texture2D::Ptr atrousFilter[2];        //A-Trous Filter

				DescriptorSet::Ptr copyTileDataSet;
			};

		}        // namespace component

		namespace update
		{
			inline auto system(ioc::Registry registry,
				const maple::component::WindowSize& winSize,
				const maple::component::RendererData& rendererData,
				const maple::component::CameraView& cameraView,
				const maple::global::component::RaytracingDescriptor& descriptor,
				const maple::global::component::AppState& appState,
				const trace::global::component::RaytraceConfig& config)
			{
				if (appState.state != EditorState::Play)
					return;

				for (auto [entity, reflection, pipeline, accumulator, atrous] : registry.getRegistry().view<
					raytraced_reflection::component::RaytracedReflection,
					raytraced_reflection::component::ReflectionPipeline,
					raytraced_reflection::component::TemporalAccumulator,
					raytraced_reflection::component::AtrousFiler
				>().each())
				{
					if (!reflection.enable)
						return;
					pipeline.pushConsts.numLights = descriptor.numLights;
					pipeline.pushConsts.approximateWithDDGI = reflection.approximateWithDDGI ? 1 : 0;

					pipeline.pushConsts.cameraPosition = { cameraView.cameraTransform->getWorldPosition(), 1.f };
					pipeline.pushConsts.viewProjInv = glm::inverse(cameraView.projView);
					pipeline.pushConsts.viewProj = cameraView.projView;
					pipeline.pushConsts.view = cameraView.view;
					pipeline.pushConsts.projInv = glm::inverse(cameraView.proj);
					pipeline.pushConsts.trim = reflection.trim;

					if (trace::isSoftTrace(config))
					{
						pipeline.pushConsts.bias = reflection.sdfBias;
						pipeline.sdfSet->setTexture("uColorSampler", rendererData.gbuffer->getBuffer(GBufferTextures::COLOR));
						pipeline.sdfSet->setTexture("uNormalSampler", rendererData.gbuffer->getBuffer(GBufferTextures::NORMALS));
						pipeline.sdfSet->setTexture("uDepthSampler", rendererData.gbuffer->getDepthBuffer());
						pipeline.sdfSet->setTexture("uPBRSampler", rendererData.gbuffer->getBuffer(GBufferTextures::PBR));
					}
					else
					{
						pipeline.pushConsts.bias = reflection.bias;
						pipeline.gbufferSet->setTexture("uNormalSampler", rendererData.gbuffer->getBuffer(GBufferTextures::NORMALS));
						pipeline.gbufferSet->setTexture("uDepthSampler", rendererData.gbuffer->getDepthBuffer());
						pipeline.gbufferSet->setTexture("uPBRSampler", rendererData.gbuffer->getBuffer(GBufferTextures::PBR));
					}

					for (auto& pushConsts : pipeline.traceShader->getPushConstants())
					{
						pushConsts.setData(&pipeline.pushConsts);
					}
				}
			}
		}        // namespace update

		namespace update_soft_trace
		{
			inline auto system(ioc::Registry registry,
				const maple::component::RendererData& rendererData,
				const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const sdf::surface::global::component::GlobalSurfacePublic& surfacePublic,
				const skybox_renderer::global::component::SkyboxData& skybox,
				const maple::global::component::AppState& appState,
				const trace::global::component::RaytraceConfig& config)
			{
				if (appState.state != EditorState::Play)
					return;

				for (auto [entity, reflection, pipeline] : registry.getRegistry().view<
					raytraced_reflection::component::RaytracedReflection,
					raytraced_reflection::component::ReflectionPipeline
				>().each()) {
					if (trace::isSoftTrace(config) && sdfPublic.texture)
					{
						pipeline.sdfSet->setTexture("uGlobalSDF", sdfPublic.texture);
						pipeline.sdfSet->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
						pipeline.sdfSet->setTexture("uSurfaceAtlasTex", surfacePublic.surfaceLightCache);
						pipeline.sdfSet->setTexture("uSurfaceAtlasDepth", surfacePublic.surfaceDepth);
						pipeline.sdfSet->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);
						pipeline.sdfSet->setStorageBuffer("SDFObjectBuffer", surfacePublic.ssboObjectBuffer);
						pipeline.sdfSet->setStorageBuffer("SDFAtlasChunkBuffer", surfacePublic.chunkBuffer);
						pipeline.sdfSet->setStorageBuffer("SDFCullObjectBuffer", surfacePublic.culledObjectsBuffer);
						pipeline.sdfSet->setTexture("uSkybox", skybox.skybox == nullptr ? rendererData.unitCube : skybox.skybox);
						pipeline.sdfSet->setUniform("UniformBufferObject", "data", &sdfPublic.globalSurfaceAtlasData);
						pipeline.sdfSet->setUniform("UniformBufferObject", "sdfData", &sdfPublic.sdfCommonData);

						for (auto [entity, volume, uniform] : registry.getRegistry().view<ddgi::component::IrradianceVolume, ddgi::component::DDGIUniform>().each())
						{
							if (volume.currentIrrdance == nullptr)
								return;

							pipeline.sdfSet->setUniform("DDGIUBO", "ddgi", &uniform);
							pipeline.sdfSet->setTexture("uIrradiance", volume.currentIrrdance);
							pipeline.sdfSet->setTexture("uDepth", volume.currentDepth);
						}
					}
				}
			}
		}        // namespace update_soft_trace

		namespace ray_trace
		{
			inline auto system(ioc::Registry registry,
				const raytracing::global::component::TopLevelAs& topLevel,
				const maple::component::WindowSize& winSize,
				const maple::component::RendererData& rendererData,
				const maple::component::CameraView& cameraView,
				const maple::global::component::RaytracingDescriptor& descriptor,
				const maple::global::component::AppState& appState,
				const trace::global::component::RaytraceConfig& config,
				const maple::global::component::RenderDevice& device)
			{
				for (auto [entity, pipeline, reflection] : registry.getRegistry().view<
					raytraced_reflection::component::ReflectionPipeline,
					raytraced_reflection::component::RaytracedReflection>().each()) {
					if (!reflection.enable)
						return;
					if (pipeline.outColor != nullptr)
						device.device->clearRenderTarget(pipeline.outColor, rendererData.commandBuffer, { 0, 0, 0, 0 });

					if (appState.state != EditorState::Play)
						return;

					if (topLevel.topLevelAs == nullptr)
						return;

					auto group = registry.getRegistry().view<ddgi::component::IrradianceVolume>().each();

					if (group.begin() != group.end() && !trace::isSoftTrace(config))
					{
						auto [entity, ddgiPipeline] = *group.begin();

						pipeline.gbufferSet->update(rendererData.commandBuffer);
						pipeline.noiseSet->update(rendererData.commandBuffer);
						pipeline.outSet->update(rendererData.commandBuffer);

						pipeline.tracePipeline->bind(rendererData.commandBuffer);
						pipeline.traceShader->bindPushConstants(rendererData.commandBuffer, pipeline.tracePipeline.get());

						Renderer::bindDescriptorSets(pipeline.tracePipeline.get(), rendererData.commandBuffer, 0,
							{ descriptor.sceneDescriptor,
							 descriptor.vboDescriptor,
							 descriptor.iboDescriptor,
							 descriptor.materialDescriptor,
							 descriptor.textureDescriptor,
							 ddgiPipeline.ddgiCommon,
							 pipeline.gbufferSet,
							 pipeline.noiseSet,
							 pipeline.outSet });

						pipeline.tracePipeline->traceRays(rendererData.commandBuffer, reflection.width, reflection.height, 1);
						reflection.output = pipeline.outColor;
					}
				}
			}
		}        // namespace ray_trace

		namespace soft_ray_trace
		{
			inline auto system(ioc::Registry registry,
				const raytracing::global::component::TopLevelAs& topLevel,
				const maple::component::WindowSize& winSize,
				const maple::component::RendererData& rendererData,
				const maple::component::CameraView& cameraView,
				const maple::global::component::RaytracingDescriptor& descriptor,
				const maple::global::component::AppState& appState,
				const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const trace::global::component::RaytraceConfig& config)
			{
				if (appState.state != EditorState::Play)
					return;

				for (auto [entity, pipeline, reflection] : registry.getRegistry().view<
					raytraced_reflection::component::ReflectionPipeline,
					raytraced_reflection::component::RaytracedReflection>().each()) {
					if (!reflection.enable)
						return;
					if (trace::isSoftTrace(config) && sdfPublic.texture )
					{
						auto group = registry.getRegistry().view<ddgi::component::IrradianceVolume>().each();
						for (auto [entity, volume] : group)
							if (volume.currentIrrdance == nullptr)
								return;

						PipelineInfo info{};
						info.shader = pipeline.sdfShader;
						auto          sdfPipeline = Pipeline::get(info);
						const int32_t dispatchGroupsX = std::ceil(reflection.width / 16.f);
						const int32_t dispatchGroupsY = std::ceil(reflection.height / 16.f);
						Renderer::dispatch(rendererData.commandBuffer, dispatchGroupsX, dispatchGroupsY, 1, sdfPipeline.get(), &pipeline.pushConsts,{ pipeline.sdfSet });
						reflection.output = pipeline.outColor;
					}
				}
			}
		}        // namespace soft_ray_trace

		namespace denoise
		{
			inline auto resetArgs(component::TemporalAccumulator& acc, const maple::component::RendererData& renderData)
			{
				acc.resetPipeline->bufferBarrier(renderData.commandBuffer,
					{ acc.denoiseDispatchBuffer,
					 acc.denoiseTileCoordsBuffer,
					 acc.copyDispatchBuffer,
					 acc.copyTileCoordsBuffer },
					false);
				acc.resetPipeline->bind(renderData.commandBuffer);
				Renderer::bindDescriptorSets(acc.resetPipeline.get(), renderData.commandBuffer, 0, { acc.indirectDescriptorSet });
				Renderer::dispatch(renderData.commandBuffer, 1, 1, 1);
				acc.resetPipeline->end(renderData.commandBuffer);
			}

			inline auto accumulation(const component::RaytracedReflection& reflection,
				component::TemporalAccumulator& accumulator,
				component::ReflectionPipeline& pipeline,
				const maple::component::RendererData& renderData,
				const maple::component::WindowSize& winSize,
				const maple::component::CameraView& cameraView)
			{
				accumulator.descriptorSets[0]->setTexture("outColor", pipeline.outputs[0]);
				accumulator.descriptorSets[0]->setTexture("moment", pipeline.currentMoments[pipeline.pingPong]);
				accumulator.descriptorSets[0]->setTexture("uHistoryOutput", pipeline.outputs[1]);        //prev
				accumulator.descriptorSets[0]->setTexture("uHistoryMoments", pipeline.currentMoments[1 - pipeline.pingPong]);

				accumulator.descriptorSets[0]->setTexture("uInput", pipeline.outColor);        //noised reflection
				accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));
				accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "prevViewProj", glm::value_ptr(cameraView.projViewOld));
				accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "cameraPos", glm::value_ptr(cameraView.cameraTransform->getWorldPosition()));

				accumulator.descriptorSets[1]->setTexture("uColorSampler", renderData.gbuffer->getBuffer(GBufferTextures::COLOR));
				accumulator.descriptorSets[1]->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				accumulator.descriptorSets[1]->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
				accumulator.descriptorSets[1]->setTexture("uPBRSampler", renderData.gbuffer->getBuffer(GBufferTextures::PBR));

				accumulator.descriptorSets[2]->setTexture("uPrevColorSampler", renderData.gbuffer->getBuffer(GBufferTextures::COLOR, true));
				accumulator.descriptorSets[2]->setTexture("uPrevNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS, true));
				accumulator.descriptorSets[2]->setTexture("uPrevDepthSampler", renderData.gbuffer->getDepthBufferPong());
				accumulator.descriptorSets[2]->setTexture("uPrevPBRSampler", renderData.gbuffer->getBuffer(GBufferTextures::PBR, true));

				for (auto set : accumulator.descriptorSets)
				{
					set->update(renderData.commandBuffer);
				}

				if (auto pushConsts = accumulator.reprojectionShader->getPushConstant(0))
				{
					accumulator.pushConsts.approximateWithDDGI = reflection.approximateWithDDGI ? 1 : 0;
					accumulator.pushConsts.alpha = reflection.alpha;
					accumulator.pushConsts.cameraDelta = cameraView.cameraDelta;
					accumulator.pushConsts.momentsAlpha = reflection.momentsAlpha;

					pushConsts->setData(&accumulator.pushConsts);
				}

				accumulator.reprojectionPipeline->bind(renderData.commandBuffer);
				accumulator.reprojectionShader->bindPushConstants(renderData.commandBuffer, accumulator.reprojectionPipeline.get());
				Renderer::bindDescriptorSets(accumulator.reprojectionPipeline.get(), renderData.commandBuffer, 0, accumulator.descriptorSets);
				auto x = static_cast<uint32_t>(ceil(float(reflection.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X)));
				auto y = static_cast<uint32_t>(ceil(float(reflection.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));
				Renderer::dispatch(renderData.commandBuffer, x, y, 1);

				accumulator.reprojectionPipeline->bufferBarrier(renderData.commandBuffer,
					{ accumulator.denoiseDispatchBuffer,
					 accumulator.denoiseTileCoordsBuffer,
					 accumulator.copyDispatchBuffer,
					 accumulator.copyTileCoordsBuffer },
					true);
			}

			inline auto atrousFilter(const component::RaytracedReflection& reflection,
				component::AtrousFiler& atrous,
				component::ReflectionPipeline& pipeline,
				const maple::component::RendererData& renderData,
				component::TemporalAccumulator& accumulator)
			{
				bool    pingPong = false;
				int32_t readIdx = 0;
				int32_t writeIdx = 1;

				atrous.gBufferSet->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				atrous.gBufferSet->setTexture("uPBRSampler", renderData.gbuffer->getBuffer(GBufferTextures::PBR));

				atrous.gBufferSet->update(renderData.commandBuffer);
				atrous.inputSet->update(renderData.commandBuffer);
				atrous.argsSet->update(renderData.commandBuffer);
				for (int32_t i = 0; i < atrous.iterations; i++)
				{
					readIdx = (int32_t)pingPong;
					writeIdx = (int32_t)!pingPong;

					renderData.renderDevice->clearRenderTarget(atrous.atrousFilter[writeIdx], renderData.commandBuffer, { 1, 1, 1, 1 });

					atrous.copyWriteDescriptorSet[writeIdx]->setTexture("outColor", atrous.atrousFilter[writeIdx]);
					atrous.copyReadDescriptorSet[readIdx]->setTexture("uInput", atrous.atrousFilter[readIdx]);

					atrous.copyWriteDescriptorSet[writeIdx]->update(renderData.commandBuffer);
					atrous.copyReadDescriptorSet[readIdx]->update(renderData.commandBuffer);

					atrous.copyTileDataSet->update(renderData.commandBuffer);

					//these coords should not denoise. so just set them as zero.
					{
						atrous.copyTilesPipeline->bind(renderData.commandBuffer);
						Renderer::bindDescriptorSets(atrous.copyTilesPipeline.get(), renderData.commandBuffer, 0, { atrous.copyWriteDescriptorSet[writeIdx], atrous.copyTileDataSet, i == 0 ? atrous.inputSet : atrous.copyReadDescriptorSet[readIdx] });
						atrous.copyTilesPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.copyDispatchBuffer.get());
						atrous.copyTilesPipeline->end(renderData.commandBuffer);
					}

					{
						atrous.atrousFilerPipeline->bind(renderData.commandBuffer);
						auto pushConsts = atrous.pushConsts;
						pushConsts.stepSize = 1 << i;
						if (auto ptr = atrous.atrousFilerPipeline->getShader()->getPushConstant(0))
						{
							pushConsts.approximateWithDDGI = reflection.approximateWithDDGI && !pipeline.firstFrame ? 1 : 0;
							pushConsts.radius = reflection.radius;
							pushConsts.phiColor = reflection.phiColor;
							pushConsts.phiNormal = reflection.phiNormal;
							pushConsts.sigmaDepth = reflection.sigmaDepth;

							ptr->setData(&pushConsts);
							atrous.atrousFilerPipeline->getShader()->bindPushConstants(renderData.commandBuffer, atrous.atrousFilerPipeline.get());
						}

						//the first time(i is zero) set the accumulation's output as current input
						Renderer::bindDescriptorSets(
							atrous.atrousFilerPipeline.get(),
							renderData.commandBuffer, 0,
							{ atrous.copyWriteDescriptorSet[writeIdx],
							 atrous.gBufferSet,
							 i == 0 ? atrous.inputSet : atrous.copyReadDescriptorSet[readIdx],
							 atrous.argsSet });

						atrous.atrousFilerPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.denoiseDispatchBuffer.get());
						atrous.atrousFilerPipeline->end(renderData.commandBuffer);
					}
					pingPong = !pingPong;

					if (i == 1)
					{
						Texture2D::copy(atrous.atrousFilter[writeIdx], pipeline.outputs[1], renderData.commandBuffer);
					}
				}
				return atrous.atrousFilter[writeIdx];
			}

			inline auto system(ioc::Registry registry,
				const maple::component::WindowSize& winSize,
				const maple::component::RendererData& rendererData,
				const maple::component::CameraView& cameraView,
				const maple::global::component::AppState& appState)
			{
				if (appState.state != EditorState::Play)
					return;

				for (auto [entity, reflection, pipeline, acc, atrous] : registry.getRegistry().view<
					component::RaytracedReflection,
					component::ReflectionPipeline,
					raytraced_reflection::component::TemporalAccumulator,
					component::AtrousFiler>().each()) {

					if (!reflection.enable)
						return;

					if (reflection.denoise)
					{
						resetArgs(acc, rendererData);
						accumulation(reflection, acc, pipeline, rendererData, winSize, cameraView);
						reflection.output = atrousFilter(reflection, atrous, pipeline, rendererData, acc);
						pipeline.pingPong = 1 - pipeline.pingPong;
					}

					pipeline.firstFrame = false;

					if (reflection.width != winSize.width || reflection.height != winSize.height)
					{
						reflection.width = winSize.width;
						reflection.height = winSize.height ;

						pipeline.outColor->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
						rendererData.renderDevice->clearRenderTarget(pipeline.outColor, rendererData.commandBuffer, { 0, 0, 0, 0 });

						for (int32_t i = 0; i < 2; i++)
						{
							pipeline.outputs[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
							pipeline.currentMoments[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
							atrous.atrousFilter[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
							atrous.copyWriteDescriptorSet[i]->setTexture("outColor", atrous.atrousFilter[i]);
							atrous.copyReadDescriptorSet[i]->setTexture("uInput", atrous.atrousFilter[1 - i]);

							rendererData.renderDevice->clearRenderTarget(atrous.atrousFilter[i], rendererData.commandBuffer, { 1, 1, 1, 1 });
							rendererData.renderDevice->clearRenderTarget(pipeline.outputs[i], rendererData.commandBuffer, { 0, 0, 0, 0 });
							rendererData.renderDevice->clearRenderTarget(pipeline.currentMoments[i], rendererData.commandBuffer, { 0, 0, 0, 0 });
						}

						auto bufferSize =
							sizeof(glm::ivec2) * static_cast<uint32_t>(ceil(float(reflection.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X))) *
							static_cast<uint32_t>(ceil(float(reflection.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));

						acc.denoiseTileCoordsBuffer->resize(bufferSize);
						acc.copyTileCoordsBuffer->resize(bufferSize);

					
						atrous.copyTileDataSet->setStorageBuffer("CopyTileData", acc.copyTileCoordsBuffer);
						acc.descriptorSets[3]->setStorageBuffer("CopyTileData", acc.copyTileCoordsBuffer);
						acc.descriptorSets[3]->setStorageBuffer("DenoiseTileData", acc.denoiseTileCoordsBuffer);
						atrous.argsSet->setStorageBuffer("DenoiseTileData", acc.denoiseTileCoordsBuffer);
						atrous.inputSet->setTexture("uInput", pipeline.outputs[0]);
					}
				}
			}
		}        // namespace denoise

		namespace frame_end
		{
			inline auto system(ioc::Registry registry, const maple::component::WindowSize& winSize, const maple::global::component::AppState& appState)
			{
				if (appState.state != EditorState::Play)
					return;

				for (auto [entity, reflection, pipeline, acc, atrous] : registry.getRegistry().view<
					component::RaytracedReflection,
					component::ReflectionPipeline,
					component::TemporalAccumulator,
					component::AtrousFiler>().each()) {

					if (!reflection.enable)
						return;

					pipeline.pushConsts.numFrames++;
				}
			}
		}        // namespace frame_end

		namespace on_game_start
		{
			inline auto system(ioc::Registry registry,
				const maple::component::WindowSize& winSize,
				const blue_noise::global::component::BlueNoise& blueNoise,
				const maple::component::RendererData& renderData,
				const maple::global::component::RenderDevice& renderDevice,
				const maple::global::component::GraphicsContext& context)
			{

				for (auto [entity, reflection, pipeline, accumulator, atrous] : registry.getRegistry().view<
					component::RaytracedReflection,
					component::ReflectionPipeline,
					component::TemporalAccumulator,
					component::AtrousFiler>().each()) {

					reflection.width = winSize.width ;
					reflection.height = winSize.height ;

					std::vector<int8_t> data;
					data.resize(sizeof(int16_t) * 4 * reflection.width * reflection.height, 0);

					pipeline.outColor = Texture2D::create();
					pipeline.outColor->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
					pipeline.outColor->update(0, 0, reflection.width, reflection.height, data.data());

					pipeline.outColor->setName("Reflection Output");

					pipeline.traceShader = Shader::create("shaders/Reflection/Reflection.shader", { {"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																								   {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																								   {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																								   {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT} });

					pipeline.sdfShader = Shader::create("shaders/SDF/SDFReflection.shader");

					pipeline.gbufferSet = DescriptorSet::create({ 6, pipeline.traceShader.get() });
					pipeline.noiseSet = DescriptorSet::create({ 7, pipeline.traceShader.get() });
					pipeline.outSet = DescriptorSet::create({ 8, pipeline.traceShader.get() });

					pipeline.sdfSet = DescriptorSet::create({ 0, pipeline.sdfShader.get() });

					pipeline.outSet->setTexture("outColor", pipeline.outColor);
					pipeline.noiseSet->setTexture("uSobolSequence", blueNoise.sobolSequence);
					pipeline.noiseSet->setTexture("uScramblingRankingTile", blueNoise.scramblingRanking[blue_noise::Blue_Noise_1SPP]);

					pipeline.sdfSet->setTexture("outColor", pipeline.outColor);
					pipeline.sdfSet->setTexture("uSobolSequence", blueNoise.sobolSequence);
					pipeline.sdfSet->setTexture("uScramblingRankingTile", blueNoise.scramblingRanking[blue_noise::Blue_Noise_1SPP]);

					PipelineInfo info;
					info.pipelineName = "Reflection-Pipeline";
					if (context.context->isRaytracingSupported()) 
					{
						info.shader = pipeline.traceShader;
						info.maxRayRecursionDepth = 1;
						pipeline.tracePipeline = Pipeline::get(info);
					}


					for (int32_t i = 0; i < 2; i++)
					{
						pipeline.outputs[i] = Texture2D::create();
						pipeline.outputs[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
						pipeline.outputs[i]->setName("Reflection Re-projection Output " + std::to_string(i));
						pipeline.outputs[i]->update(0, 0, reflection.width, reflection.height, data.data());

						pipeline.currentMoments[i] = Texture2D::create();
						pipeline.currentMoments[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
						pipeline.currentMoments[i]->setName("Reflection Re-projection Moments " + std::to_string(i));
						pipeline.currentMoments[i]->update(0, 0, reflection.width, reflection.height, data.data());
					}

					//#########################################################################################################

					auto bufferSize =
						sizeof(glm::ivec2) * static_cast<uint32_t>(ceil(float(reflection.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X))) *
						static_cast<uint32_t>(ceil(float(reflection.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));

					accumulator.resetArgsShader = Shader::create("shaders/Reflection/DenoiseReset.shader");

					accumulator.denoiseTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
					accumulator.denoiseDispatchBuffer = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{ true, MemoryUsage::MEMORY_USAGE_GPU_ONLY });

					accumulator.copyTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
					accumulator.copyDispatchBuffer = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{ true, MemoryUsage::MEMORY_USAGE_GPU_ONLY });

					{
						PipelineInfo info;
						info.shader = accumulator.resetArgsShader;
						info.pipelineName = "Reflection-DenoiseReset";
						accumulator.resetPipeline = Pipeline::get(info);

						accumulator.reprojectionShader = Shader::create("shaders/Reflection/Reprojection.shader");
						constexpr char* str[4] = { "Accumulation", "GBuffer", "PrevGBuffer", "Args" };
						accumulator.descriptorSets.resize(4);
						for (uint32_t i = 0; i < 4; i++)
						{
							accumulator.descriptorSets[i] = DescriptorSet::create({ i, accumulator.reprojectionShader.get() });
							accumulator.descriptorSets[i]->setName(str[i]);
						}

						accumulator.descriptorSets[3]->setStorageBuffer("DenoiseTileData", accumulator.denoiseTileCoordsBuffer);
						accumulator.descriptorSets[3]->setStorageBuffer("DenoiseTileDispatchArgs", accumulator.denoiseDispatchBuffer);
						accumulator.descriptorSets[3]->setStorageBuffer("CopyTileData", accumulator.copyTileCoordsBuffer);
						accumulator.descriptorSets[3]->setStorageBuffer("CopyTileDispatchArgs", accumulator.copyDispatchBuffer);
						accumulator.descriptorSets[3]->update(renderData.commandBuffer);

						info.shader = accumulator.reprojectionShader;
						info.pipelineName = "Reflection-Reprojection";
						accumulator.reprojectionPipeline = Pipeline::get(info);
					}
					//###############

					PipelineInfo info2;
					info2.pipelineName = "Reflection-Reset-Args";
					info2.shader = accumulator.resetArgsShader;
					accumulator.resetPipeline = Pipeline::get(info2);

					accumulator.indirectDescriptorSet = DescriptorSet::create({ 0, accumulator.resetArgsShader.get() });
					accumulator.indirectDescriptorSet->setStorageBuffer("DenoiseTileDispatchArgs", accumulator.denoiseDispatchBuffer);
					accumulator.indirectDescriptorSet->setStorageBuffer("CopyTileDispatchArgs", accumulator.copyDispatchBuffer);
					accumulator.indirectDescriptorSet->update(renderData.commandBuffer);

					{
						atrous.atrousFilerShader = Shader::create("shaders/Reflection/DenoiseAtrous.shader");
						atrous.copyTilesShader = Shader::create("shaders/Reflection/DenoiseCopyTiles.shader");

						PipelineInfo info1;

						info1.pipelineName = "Reflection-Atrous-Filer Pipeline";
						info1.shader = atrous.atrousFilerShader;
						atrous.atrousFilerPipeline = Pipeline::get(info1);

						info1.pipelineName = "Reflection-Atrous-Copy Tiles Pipeline";
						info1.shader = atrous.copyTilesShader;
						atrous.copyTilesPipeline = Pipeline::get(info1);

						atrous.gBufferSet = DescriptorSet::create({ 1, atrous.atrousFilerShader.get() });
						atrous.argsSet = DescriptorSet::create({ 3, atrous.atrousFilerShader.get() });
						atrous.argsSet->setStorageBuffer("DenoiseTileData", accumulator.denoiseTileCoordsBuffer);
						atrous.argsSet->update(renderData.commandBuffer);

						for (uint32_t i = 0; i < 2; i++)
						{
							atrous.atrousFilter[i] = Texture2D::create();
							atrous.atrousFilter[i]->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
							atrous.atrousFilter[i]->setName("A-Trous Filter " + std::to_string(i));

							atrous.copyWriteDescriptorSet[i] = DescriptorSet::create({ 0, atrous.atrousFilerShader.get() });
							atrous.copyWriteDescriptorSet[i]->setTexture("outColor", atrous.atrousFilter[i]);
							atrous.copyWriteDescriptorSet[i]->setName("Atrous-Write-Descriptor-" + std::to_string(i));

							atrous.copyReadDescriptorSet[i] = DescriptorSet::create({ 2, atrous.atrousFilerShader.get() });
							atrous.copyReadDescriptorSet[i]->setName("Atrous-Read-Descriptor-" + std::to_string(i));
						}

						atrous.copyTileDataSet = DescriptorSet::create({ 1, atrous.copyTilesShader.get() });
						atrous.copyTileDataSet->setStorageBuffer("CopyTileData", accumulator.copyTileCoordsBuffer);
						atrous.copyTileDataSet->setName("CopyTileData");

						atrous.inputSet = DescriptorSet::create({ 2, atrous.atrousFilerShader.get() });
						atrous.inputSet->setTexture("uInput", pipeline.outputs[0]);
					}
				}
			}
		}        // namespace on_game_start

		auto registerRaytracedReflection(SystemQueue& begin, SystemQueue& queue, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->addDependency<raytraced_reflection::component::RaytracedReflection, component::TemporalAccumulator>();
			builder->addDependency<raytraced_reflection::component::RaytracedReflection, component::ReflectionPipeline>();
			builder->addDependency<raytraced_reflection::component::RaytracedReflection, component::AtrousFiler>();
			builder->registerGameStart<on_game_start::system>();
			builder->registerWithinQueue<raytraced_reflection::update::system>(begin);
			builder->registerWithinQueue<raytraced_reflection::update_soft_trace::system>(begin);
			builder->registerWithinQueue<raytraced_reflection::ray_trace::system>(queue);
			builder->registerWithinQueue<raytraced_reflection::soft_ray_trace::system>(queue);
			builder->registerWithinQueue<raytraced_reflection::denoise::system>(queue);
			builder->registerSystemInFrameEnd<raytraced_reflection::frame_end::system>();
		}
	};        // namespace raytraced_reflection
};            // namespace maple