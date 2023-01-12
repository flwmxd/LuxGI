//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GlobalDistanceField.h"
#include "MeshDistanceField.h"

#include "Engine/Mesh.h"
#include "Engine/Renderer/RendererData.h"

#include "Engine/Core.h"
#include "Engine/GBuffer.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "ImGui/ImGuiHelpers.h"
#include "Math/BoundingBox.h"
#include "Math/BoundingSphere.h"
#include "Math/MathUtils.h"

#include "RHI/DescriptorPool.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/Shader.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"

#include "Others/Timer.h"

#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

#include "IO/File.h"

#include "GlobalSurfaceAtlas.h"

#include <glm/glm.hpp>
#include <imgui.h>
#include "ImGui/ImNotification.h"
#include <map>

namespace maple::sdf
{
	namespace
	{
		enum DebugID
		{
			DrawGlobalSDF,
			DrawSDFColor,
			DrawSDFNormal,
			Length
		};

		static const char* NAMES[] = {
			"Draw Global SDF",
			"Draw SDF Color",
			"Draw SDF Normal" };

		enum DebugGBufferID
		{
			SurfaceEmissive,
			SurfaceGBuffer0,
			SurfaceGBuffer1,
			SurfaceGBuffer2,
			SurfaceLightCache,
			DebugGBufferIDLength
		};

		static const char* NAMES_GBUFFER[] = {
			"SurfaceEmissive",
			"SurfaceGBuffer0",
			"SurfaceGBuffer1",
			"SurfaceGBuffer2",
			"SurfaceLightCache",
		};

		inline auto flatten(const glm::ivec3& position, const glm::ivec3& resolution)
		{
			return position.x + position.y * resolution.x + position.z * resolution.x * resolution.y;
		};

		constexpr int32_t CONSTS_SDF_RASTERIZE_MODEL_MAX_COUNT = 28;
		constexpr int32_t CONSTS_SDF_RASTERIZE_MODEL_SET_MAX_COUNT = 512;
		constexpr int32_t CONSTS_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT = 2;
		constexpr int32_t CONSTS_SDF_RASTERIZE_GROUP_SIZE = 8;
		constexpr int32_t CONSTS_SDF_RASTERIZE_CHUNK_SIZE = 32;
		constexpr int32_t CONSTS_SDF_RASTERIZE_MIP_FACTOR = 4;
		constexpr int32_t CONSTS_SDF_MIP_GROUP_SIZE = 4;
		constexpr int32_t CONSTS_SDF_RASTERIZE_CHUNK_MARGIN = 4;
		constexpr int32_t CONSTS_SDF_MIP_FLOODS = CONSTS_SDF_RASTERIZE_MIP_FACTOR + 1;
		constexpr int16_t RASTERIZE_CHUNK_KEY_HASH_RESOLUTION = CONSTS_SDF_RASTERIZE_CHUNK_SIZE;

		struct GlobalSDFMipmapPushConsts
		{
			int32_t globalSDFResolution;
			int32_t mipmapcoordScale;
			int32_t cascadeTexOffsetX;
			int32_t cascadeMipMapOffsetX;
			float   maxDistance;
		};

		struct ObjectRasterizeData
		{
			glm::mat4 worldToVolume;
			glm::mat4 volumeToWorld;
			glm::vec3 volumeToUVWMul;
			float     mipOffset;
			glm::vec3 volumeToUVWAdd;
			float     decodeMul;
			glm::vec3 volumeLocalBoundsExtent;
			float     decodeAdd;
		};

		struct Chunk
		{
			bool dynamic;
			uint16_t modelsCount;
			uint32_t models[CONSTS_SDF_RASTERIZE_MODEL_MAX_COUNT];

			Chunk()
			{
				modelsCount = 0;
				dynamic = false;
			}
		};

		struct ChunkKey
		{
			uint32_t   hash;
			int32_t    layer;
			glm::ivec3 coord;

			inline auto nextLayer()
			{
				layer++;
				hash += RASTERIZE_CHUNK_KEY_HASH_RESOLUTION * RASTERIZE_CHUNK_KEY_HASH_RESOLUTION * RASTERIZE_CHUNK_KEY_HASH_RESOLUTION;
			}

			friend auto operator==(const ChunkKey& a, const ChunkKey& b)
			{
				return a.hash == b.hash && a.coord == b.coord && a.layer == b.layer;
			}
		};

		struct HashFunc
		{
			size_t operator()(const ChunkKey& i) const
			{
				return i.hash;
			}
		};

		struct CascadeData
		{
			glm::vec3                                       position;
			float                                           voxelSize;
			BoundingBox                                     bounds;
			std::unordered_set<ChunkKey, HashFunc> nonEmptyChunks;
			std::unordered_set<ChunkKey, HashFunc> staticChunks;
			std::set<std::string>                           objects;
		};

		struct ModelsData
		{
			glm::vec3 cascadecoordToPosMul;
			float     maxDistance;
			glm::vec3 cascadecoordToPosAdd;
			int32_t   cascadeResolution;
			int32_t   cascadeIndex;
			glm::vec3 padding;
		};

		struct RasterizeConsts
		{
			glm::ivec3 chunkcoord;
			int32_t    objectsCount;
			uint32_t   objects[CONSTS_SDF_RASTERIZE_MODEL_MAX_COUNT];
		};

		std::unordered_map<ChunkKey, Chunk, HashFunc> chunksCache;

		inline auto getQuality(Quality quality, int32_t& resolution, int32_t& cascadesCount)
		{
			switch (quality)
			{
			case Quality::Test:
				resolution = 192;
				cascadesCount = 2;
				break;
			}
		}

		inline auto getChunkId(float voxelSize, const BoundingBox& cascadeBounds, const BoundingBox& objectBounds, glm::ivec3& objectChunkMin, glm::ivec3& objectChunkMax)
		{
			BoundingBox objectBoundsCascade;
			const float objectMargin = voxelSize * CONSTS_SDF_RASTERIZE_CHUNK_MARGIN;
			auto        cascadeBoundsBias = cascadeBounds;
			cascadeBoundsBias.min += 0.1f;
			cascadeBoundsBias.max -= 0.1f;
			objectBoundsCascade.min = objectBounds.min - objectMargin;
			objectBoundsCascade.max = objectBounds.max + objectMargin;
			glm::clamp(objectBoundsCascade.min, cascadeBoundsBias.min, cascadeBoundsBias.max);
			glm::clamp(objectBoundsCascade.max, cascadeBoundsBias.min, cascadeBoundsBias.max);

			objectBoundsCascade.min -= cascadeBoundsBias.min;
			objectBoundsCascade.max -= cascadeBoundsBias.min;        //relative to cascade bounds.*/
			const float chunkSize = voxelSize * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;
			objectChunkMin = { objectBoundsCascade.min / chunkSize };        //compute chunk id.
			objectChunkMax = { objectBoundsCascade.max / chunkSize };
		}

	}        // namespace

	namespace global::component
	{
		struct GlobalDistanceField
		{
			Quality                   quality = Quality::Test;
			int32_t                   resolution = 0;
			std::vector<CascadeData>  cascadeData;
			Texture3D::Ptr            mipTempTexture;
			uint32_t                  objectsBufferCount = 0;
			std::vector<Texture::Ptr> sdfTextrues;
			StorageBuffer::Ptr        sdfBuffer;
			int32_t                   rasterizeChunks = 0;

			std::vector<ObjectRasterizeData> objects;

			Shader::Ptr        shader;
			Pipeline::Ptr      pipeline;
			Shader::Ptr        shaderNoRead;
			Pipeline::Ptr      pipelineNoRead;
			DescriptorSet::Ptr sets[4];

			//##########################################

			Shader::Ptr        shaderMip;
			Pipeline::Ptr      pipelineMip;
			DescriptorSet::Ptr set0;
			DescriptorSet::Ptr set1;        //for temp
			DescriptorSet::Ptr set2;        //for temp

			//####################################
			Shader::Ptr        clearShader;
			Pipeline::Ptr      clearPipeline;
			DescriptorSet::Ptr clearSets;

			bool showSDF = false;
			bool showGrid = false;

			int32_t drawIdx = 0;
			int32_t surfaceIndex = 4;
			float   stepScale = 1;

			bool needBake = false;
		};

		struct SDFVisualizer
		{
			std::shared_ptr<Shader>                     shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
		};
	}        // namespace global::component

	namespace draw_voxel
	{
		inline auto system(maple::component::RendererData& rendererData,
			global::component::SDFVisualizer& pipline,
			global::component::GlobalDistanceField& globalSDF,
			global::component::GlobalDistanceFieldPublic& sdfPublic,
			surface::global::component::GlobalSurfacePublic& surfacePublic,
			const skybox_renderer::global::component::SkyboxData& skyboxData,
			const maple::component::CameraView& cameraView)
		{
			sdfPublic.sdfCommonData.nearPlane = cameraView.nearPlane;
			sdfPublic.sdfCommonData.farPlane = cameraView.farPlane;
			sdfPublic.sdfCommonData.resolution = globalSDF.resolution;
			sdfPublic.sdfCommonData.cascadesCount = globalSDF.cascadeData.size();

			if (sdfPublic.texture == nullptr || !globalSDF.showSDF || surfacePublic.ssboTileBuffer == nullptr)
				return;

			struct Ubo
			{
				glm::vec4              viewFrustumWorldRays[4];
				GlobalSurfaceAtlasData data;
				GlobalSDFData          sdfData;
				int32_t                debugType;
				float                  stepScale;
			} ubo;

			ubo.data = sdfPublic.globalSurfaceAtlasData;
			ubo.sdfData = sdfPublic.sdfCommonData;
			ubo.debugType = globalSDF.drawIdx;
			ubo.stepScale = globalSDF.stepScale;

			for (auto i = 0; i < 4; i++)
				ubo.viewFrustumWorldRays[i] = glm::vec4(cameraView.frustum.getVertices()[4 + i], 1);

			pipline.descriptors[0]->setTexture("uGlobalSDF", sdfPublic.texture);
			pipline.descriptors[0]->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
			pipline.descriptors[0]->setTexture("uSurfaceAtlasDepth", surfacePublic.surfaceDepth);
			pipline.descriptors[0]->setTexture("uSkybox", skyboxData.skybox == nullptr ? rendererData.unitCube : skyboxData.skybox);
			pipline.descriptors[0]->setTexture("uSurfaceAtlasTex", sdf::getTexture(globalSDF.surfaceIndex, surfacePublic));

			pipline.descriptors[0]->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);
			pipline.descriptors[0]->setStorageBuffer("SDFObjectBuffer", surfacePublic.ssboObjectBuffer);
			pipline.descriptors[0]->setStorageBuffer("SDFAtlasChunkBuffer", surfacePublic.chunkBuffer);
			pipline.descriptors[0]->setStorageBuffer("SDFCullObjectBuffer", surfacePublic.culledObjectsBuffer);

			pipline.descriptors[0]->setTexture("uScreen", rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN));
			pipline.descriptors[0]->setUniformBufferData("UniformBufferObject", &ubo);

			const uint32_t dispatchX = static_cast<uint32_t>(std::ceil(float(rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN)->getWidth()) / 8.f));
			const uint32_t dispatchY = static_cast<uint32_t>(std::ceil(float(rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN)->getHeight()) / 8.f));


			PipelineInfo pipelineInfo;
			pipelineInfo.shader = pipline.shader;
			pipelineInfo.pipelineName = "SDFVisualizer";
			auto pipeline = Pipeline::get(pipelineInfo);
			Renderer::dispatch(rendererData.commandBuffer, dispatchX, dispatchY, 1, pipeline.get(), nullptr, pipline.descriptors);
		}
	}        // namespace draw_voxel

	namespace generate_sdf
	{
		inline auto system(ioc::Registry registry, sdf::global::component::GlobalDistanceField& globalSDF, maple::component::RendererData& data, const global::component::GlobalDistanceFieldPublic& sdfPublic)
		{
			if (globalSDF.needBake)
			{
				JobSystem::Context context;
				Timer              timer;
				timer.start();

				auto group = registry.getRegistry().view<
					maple::component::NameComponent,
					maple::component::MeshRenderer,
					maple::component::Transform,
					component::MeshDistanceField
				>();

				for (auto entity : group)
				{
					JobSystem::execute(context, [=](JobSystem::JobDispatchArgs args) {
						auto [name, mesh, transform, sdf] = group.get(entity);
						if (sdf.buffer == nullptr && !File::fileExists(sdf.bakedPath))
						{
							LOGI("Baking MeshDistanceField : {} ", name.name);
							sdf::baker::bake(mesh.mesh, sdfPublic.config, sdf, transform);
						}
						});
				}
				JobSystem::wait(context);
				for (auto [entity, name, mesh, transform, sdf] : group.each())
				{
					ImNotification::makeNotification("GlobalSDF", "Loading MeshDistanceField : " + sdf.bakedPath, ImNotification::Type::Info);
					sdf::baker::load(sdf, data.commandBuffer);
				}
				auto elapsed = timer.stop();
				LOGI("total cost : {}", elapsed);
				ImNotification::makeNotification("GlobalSDF", "Loaded All MeshDistanceFields, Please Click Run DDGI! ", ImNotification::Type::Success, 10000);
				globalSDF.needBake = false;
			}
		}
	}        // namespace generate_sdf

	namespace on_scene_changed
	{
		inline auto system(ioc::Registry registry,
			sdf::global::component::GlobalDistanceField& globalSDF,
			maple::global::component::SceneTransformChanged& changed)
		{
			auto meshGroup = registry.getRegistry().view<
				maple::component::MeshRenderer,
				maple::component::Transform,
				component::MeshDistanceField
			>();

			if (meshGroup.begin() == meshGroup.end())
				return;

			if (changed.dirty)
			{
				for (auto id : changed.entities)
				{
					if (meshGroup.contains(id))
					{
						auto [render, transform, sdf] = meshGroup.get(id);
						auto        objectBounds = sdf.aabb.transform(transform.getWorldMatrix());
						BoundingBox objectBoundsCascade;
						for (auto& cascade : globalSDF.cascadeData)
						{
							glm::ivec3 objectChunkMin;
							glm::ivec3 objectChunkMax;
							getChunkId(cascade.voxelSize, cascade.bounds, objectBounds, objectChunkMin, objectChunkMax);

							ChunkKey key;
							key.layer = 0;

							for (key.coord.z = objectChunkMin.z; key.coord.z <= objectChunkMax.z; key.coord.z++)
							{
								for (key.coord.y = objectChunkMin.y; key.coord.y <= objectChunkMax.y; key.coord.y++)
								{
									for (key.coord.x = objectChunkMin.x; key.coord.x <= objectChunkMax.x; key.coord.x++)
									{
										key.hash = flatten(key.coord, glm::ivec3{ RASTERIZE_CHUNK_KEY_HASH_RESOLUTION });
										cascade.staticChunks.erase(key);
									}
								}
							}
						}
					}
				}
			}
		}        // namespace chunk_calculate
	}            // namespace on_scene_changed

	namespace on_imgui
	{
		inline auto system(ioc::Registry registry, global::component::GlobalDistanceField& globalSDF,
			global::component::GlobalDistanceFieldPublic& sdfPublic,
			const global::component::GlobalDistanceField& sdfData
		)
		{

			auto meshGroup = registry.getRegistry().view<maple::component::MeshRenderer, maple::component::Transform>(entt::exclude<component::MeshDistanceField>);

			if (ImGui::Begin("GlobalDistanceField"))
			{
				if (ImGui::Button("Generate"))
				{
					globalSDF.needBake = true;
					for (auto entity : meshGroup)
					{
						registry.addComponent<sdf::component::MeshDistanceField>(entity);
					}
				}

				ImGui::Columns(2);
				ImGuiHelper::property("Debug Global SDF", globalSDF.showSDF);

				if (globalSDF.showSDF)
				{
					if (auto id = ImGuiHelper::combox("Debug Type", NAMES, DebugID::Length, globalSDF.drawIdx); id != -1)
					{
						globalSDF.drawIdx = id;
					}

					if (auto id = ImGuiHelper::combox("Surface Type", NAMES_GBUFFER, DebugGBufferID::DebugGBufferIDLength, globalSDF.surfaceIndex); id != -1)
					{
						globalSDF.surfaceIndex = id;
					}

					ImGuiHelper::property("Step Scale", globalSDF.stepScale, 0.1, 10);
				}
				ImGuiHelper::property("SDF GI Distance", sdfPublic.giDistance, 1, 15000, ImGuiHelper::PropertyFlag::DragFloat);
				ImGuiHelper::property("Global Scale", sdfPublic.gloalScale, 0.001, 10, ImGuiHelper::PropertyFlag::DragFloat);
				ImGui::Columns(1);
			}
			ImGui::End();
		}
	}        // namespace on_imgui

	namespace merge_CONSTS_sdf
	{
		// how many distance represented in a voxel.
		inline auto chunkCalculate(
			maple::component::MeshRenderer& render,
			maple::component::Transform& transform,
			component::MeshDistanceField& sdf,
			float voxelSize, const BoundingBox& cascadeBounds, const BoundingBox& sdfBounds,
			global::component::GlobalDistanceField& sdfData, const global::component::GlobalDistanceFieldPublic& sdfPublic,
			std::vector<ObjectRasterizeData>& buffer, int32_t cascadeLevel)
		{
			auto objectBounds = sdfBounds.transform(transform.getWorldMatrix());

			glm::ivec3 objectChunkMin;
			glm::ivec3 objectChunkMax;
			getChunkId(voxelSize, cascadeBounds, objectBounds, objectChunkMin, objectChunkMax);

			BoundingBox localVolumeBounds = sdfBounds;
			glm::vec3   volumeCenter = localVolumeBounds.center();

			glm::mat4 worldToLocal = transform.getWorldMatrixInverse();
			glm::mat4 worldToVolume = worldToLocal * glm::translate(glm::mat4(1.f), -volumeCenter);
			glm::mat4 volumeToWorld = glm::inverse(worldToVolume);

			int32_t mipLevelIndex = std::min(cascadeLevel, 2);

			glm::vec3 volumeToUVWMul = sdf.localToUVWMul;
			glm::vec3 volumeToUVWAdd = sdf.localToUVWAdd + volumeCenter * sdf.localToUVWMul;

			uint16_t objectIndex = sdfData.objectsBufferCount++;

			ObjectRasterizeData& objectData = buffer.emplace_back();
			objectData.worldToVolume = worldToVolume;
			objectData.volumeToWorld = volumeToWorld;
			objectData.volumeLocalBoundsExtent = localVolumeBounds.size() / 2.f;
			objectData.volumeToUVWMul = volumeToUVWMul;
			objectData.volumeToUVWAdd = volumeToUVWAdd;
			objectData.mipOffset = (float)mipLevelIndex;
			objectData.decodeMul = sdf.maxDistance;
			objectData.decodeAdd = -sdf.maxDistance;

			ChunkKey key;
			bool              dynamicObj = false;

			for (key.coord.z = objectChunkMin.z; key.coord.z <= objectChunkMax.z; key.coord.z++)
			{
				for (key.coord.y = objectChunkMin.y; key.coord.y <= objectChunkMax.y; key.coord.y++)
				{
					for (key.coord.x = objectChunkMin.x; key.coord.x <= objectChunkMax.x; key.coord.x++)
					{
						key.layer = 0;
						key.hash = flatten(key.coord, glm::ivec3{ RASTERIZE_CHUNK_KEY_HASH_RESOLUTION });
						auto& chunk = chunksCache[key];
						chunk.dynamic |= false;        //assume every thing is static.

						// Move to the next layer if chunk has overflown
						while (chunk.modelsCount == CONSTS_SDF_RASTERIZE_MODEL_MAX_COUNT)
						{
							key.nextLayer();
							chunk = chunksCache[key];
						}
						chunk.models[chunk.modelsCount++] = objectIndex;
					}
				}
			}

			if (!dynamicObj)
			{
				sdfData.sdfTextrues.emplace_back(sdf.buffer);
			}
		}

		inline auto fillFlood(uint32_t mipDispatchGroups,
			uint32_t cascadeIndex,
			uint32_t mipMapRes,
			GlobalSDFMipmapPushConsts& pushConsts,
			global::component::GlobalDistanceField& sdfData,
			global::component::GlobalDistanceFieldPublic& sdfPublic,
			const maple::component::RendererData& renderData)
		{
			//1. generate mip1.

			sdfData.set1->setTexture("uGlobalSDF", sdfPublic.mipTexture);
			sdfData.set1->setTexture("uGlobalMipSDF", sdfData.mipTempTexture);
			sdfData.set2->setTexture("uGlobalSDF", sdfData.mipTempTexture);
			sdfData.set2->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);

			pushConsts.globalSDFResolution = mipMapRes;
			pushConsts.mipmapcoordScale = 1;
			for (auto i = 1; i < 5; i++)
			{
				if ((i & 1) == 1)
				{
					//Mip -> Tmp
					pushConsts.cascadeTexOffsetX = cascadeIndex * mipMapRes;
					pushConsts.cascadeMipMapOffsetX = 0;
				}
				else
				{
					// Tmp -> Mip
					pushConsts.cascadeTexOffsetX = 0;
					pushConsts.cascadeMipMapOffsetX = cascadeIndex * mipMapRes;
				}
				DescriptorSet::Ptr set = (i & 1) == 1 ? sdfData.set1 : sdfData.set2;
				Renderer::dispatch(renderData.commandBuffer, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups, sdfData.pipelineMip.get(), &pushConsts, { set });

				Renderer::memoryBarrier(renderData.commandBuffer, ShaderType::Compute, ShaderType::Compute, AccessFlags::ReadWrite, AccessFlags::ReadWrite);
			}
		}

		inline auto system(ioc::Registry registry,
			maple::component::CameraView& cameraView,
			maple::global::component::RenderDevice& renderDevice,
			maple::component::RendererData& renderData,
			global::component::GlobalDistanceField& sdfData,
			global::component::GlobalDistanceFieldPublic& sdfPublic,
			const global::component::SDFVisualizer& visualizer,
			const maple::global::component::Profiler& profiler)
		{
			sdfPublic.sdfSet = visualizer.descriptors[0];

			auto group = registry.getRegistry().view<
				maple::component::MeshRenderer,
				maple::component::Transform,
				component::MeshDistanceField
			>();

			if (group.begin() == group.end())
				return;

			int32_t sdfResolution;
			int32_t cascadesCount;
			getQuality(sdfData.quality, sdfResolution, cascadesCount);
			const int32_t resolutionMip = math::divideAndRoundUp(sdfResolution, CONSTS_SDF_RASTERIZE_MIP_FACTOR);
			const float   distance = std::min(15000.0f, sdfPublic.giDistance) * sdfPublic.gloalScale;        //append gi distance....
			const float   cascadesDistanceScales[] = { 1.0f, 2.5f, 5.0f, 10.0f };
			const float   distanceExtent = distance / cascadesDistanceScales[cascadesCount - 1];

			bool updated = false;

			//create cascade texture..
			if (sdfData.cascadeData.size() != cascadesCount || sdfData.resolution != sdfResolution)
			{
				sdfData.cascadeData.resize(cascadesCount);
				sdfData.resolution = sdfResolution;
				updated = true;
				uint32_t width = sdfResolution * cascadesCount;

				if (sdfPublic.texture == nullptr || sdfPublic.texture->getWidth() != width)
				{
					sdfPublic.texture = Texture3D::create(width, sdfResolution, sdfResolution, { TextureFormat::R16F, TextureFilter::Linear, TextureWrap::ClampToEdge });
				}

				width = resolutionMip * cascadesCount;
				if (sdfPublic.mipTexture == nullptr || sdfPublic.mipTexture->getWidth() != width)
				{
					sdfPublic.mipTexture = Texture3D::create(width, resolutionMip, resolutionMip, { TextureFormat::R16F, TextureFilter::Linear, TextureWrap::ClampToEdge });
					sdfData.mipTempTexture = Texture3D::create(resolutionMip, resolutionMip, resolutionMip, { TextureFormat::R16F, TextureFilter::Linear, TextureWrap::ClampToEdge });
				}
			}

			if (updated)
			{
				for (auto& cascade : sdfData.cascadeData)
				{
					cascade.nonEmptyChunks.clear();
					cascade.staticChunks.clear();
				}
				renderDevice.device->clearRenderTarget(sdfPublic.texture, renderData.commandBuffer, glm::vec4(1.f));
				renderDevice.device->clearRenderTarget(sdfPublic.mipTexture, renderData.commandBuffer, glm::vec4(1.f));
				renderDevice.device->clearRenderTarget(sdfData.mipTempTexture, renderData.commandBuffer, glm::vec4(1.f));
			}

			auto      viewPosition = glm::vec3(0.f);        //cameraView.cameraTransform->getWorldPosition();
			glm::vec3 viewDirection = maple::FORWARD;        //cameraView.cameraTransform->getForwardDirection();
			{
				glm::vec3       viewDirection = maple::FORWARD;
				const float     cascade0Distance = distanceExtent * cascadesDistanceScales[0];
				const glm::vec2 viewRayHit = math::lineHitsAABB(viewPosition, viewPosition + viewDirection * (cascade0Distance * 2.0f), viewPosition - cascade0Distance, viewPosition + cascade0Distance);
				const float     viewOriginOffset = (float)viewRayHit.y * cascade0Distance * 0.6f;
				viewPosition += viewDirection * viewOriginOffset;
			}

			const int32_t rasterizeChunks = (int32_t)std::ceilf((float)sdfResolution / (float)CONSTS_SDF_RASTERIZE_CHUNK_SIZE);
			sdfData.rasterizeChunks = rasterizeChunks;
			const bool useCache = !updated;

			const uint64_t cascadeFrequencies[] = { 2, 3, 5, 11 };
			for (int32_t cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
			{
				// Reduce frequency of the updates
				if (useCache && (profiler.frameCount % cascadeFrequencies[cascadeIndex]) != 0)
					continue;

				auto& cascade = sdfData.cascadeData[cascadeIndex];
				const float cascadeDistance = distanceExtent * cascadesDistanceScales[cascadeIndex];
				const float cascadeMaxDistance = cascadeDistance * 2;
				const float cascadeVoxelSize = cascadeMaxDistance / (float)sdfResolution;                //how many distance represented in a voxel.
				const float cascadeChunkSize = cascadeVoxelSize * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;        // 32 voxel as a chunk.

				const glm::vec3 center = glm::floor(viewPosition / cascadeChunkSize) * cascadeChunkSize;
				BoundingBox     cascadeBounds(center - cascadeDistance, center + cascadeDistance);
				const float     minObjectRadius = sdfPublic.minObjectRadius * sdfPublic.gloalScale;

				{
					chunksCache.clear();
					sdfData.sdfTextrues.clear();
				}

				if (!(useCache && math::equals(cascade.position, center, glm::vec3(cascadeVoxelSize))))
				{
					cascade.objects.clear();
					cascade.staticChunks.clear();
				}

				cascade.position = center;
				cascade.voxelSize = cascadeVoxelSize;
				cascade.bounds = cascadeBounds;

				sdfData.objectsBufferCount = 0;
				std::vector<ObjectRasterizeData> data;

				for (auto entity : group.each())
				{
					auto [_, render, transform, sdf] = entity;
					if (sdf.buffer == nullptr)
						return;
					BoundingBox    objectBounds = sdf.aabb.transform(transform.getWorldMatrix());
					BoundingSphere sphereBox = objectBounds;
					if (cascade.bounds.intersectsWithSphere(sphereBox) && sphereBox.radius >= minObjectRadius)
					{
						chunkCalculate(render, transform, sdf, cascade.voxelSize, cascadeBounds, sdf.aabb, sdfData, sdfPublic, data, cascadeIndex);
					}
				}

				constexpr int32_t chunkDispatchGroups = CONSTS_SDF_RASTERIZE_CHUNK_SIZE / CONSTS_SDF_RASTERIZE_GROUP_SIZE;        //4

				sdfData.sdfBuffer->setData(sizeof(ObjectRasterizeData) * data.size(), data.data());
				sdfData.clearSets->setTexture("uGlobalSDF", sdfPublic.texture);
				RasterizeConsts consts{};
				bool            anyChunkDispatch = false;
				struct PushConsts
				{
					glm::ivec3 chunkCoord;
					int32_t    cascadeOffset;
				};
				PushConsts pushConsts = { {}, cascadeIndex * sdfData.resolution };

				{
					for (auto it = cascade.nonEmptyChunks.begin(); it != cascade.nonEmptyChunks.end();)
					{        //when chunk merge or expand (like move object), so clear that chunk..
						if (chunksCache.count(*it) > 0)
						{
							++it;
							continue;
						}

						it = cascade.nonEmptyChunks.erase(it);
						if (it == cascade.nonEmptyChunks.end())
							break;

						consts.chunkcoord = it->coord * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;
						Renderer::dispatch(
							renderData.commandBuffer,
							chunkDispatchGroups,
							chunkDispatchGroups,
							chunkDispatchGroups, sdfData.clearPipeline.get(), &pushConsts, { sdfData.clearSets });
						anyChunkDispatch = true;
					}
				}

				for (auto it = chunksCache.begin(); it != chunksCache.end(); ++it)
				{
					if (it->first.layer == 0)
					{
						if (it->second.dynamic)
						{
							cascade.staticChunks.erase(it->first);
						}
						else if (cascade.staticChunks.count(it->first) > 0)
						{
							auto key = it->first;
							while (chunksCache.erase(key) > 0)
								key.nextLayer();
						}
						else
						{
							cascade.staticChunks.emplace(it->first);
						}
					}
				}

				ModelsData modelData;
				modelData.cascadecoordToPosMul = cascadeBounds.size() / (float)sdfData.resolution;
				modelData.cascadecoordToPosAdd = cascadeBounds.min + cascadeVoxelSize * 0.5f; //convert to voxel center.
				modelData.maxDistance = cascadeMaxDistance;
				modelData.cascadeResolution = sdfData.resolution;
				modelData.cascadeIndex = cascadeIndex;

				sdfData.sets[cascadeIndex]->setStorageBuffer("SDFObjectData", sdfData.sdfBuffer);
				sdfData.sets[cascadeIndex]->setTexture("uGlobalSDF", sdfPublic.texture);
				sdfData.sets[cascadeIndex]->setUniformBufferData("ModelsRasterizeData", &modelData, true);
				sdfData.sets[cascadeIndex]->setTexture("uMeshSDF", sdfData.sdfTextrues);

				for (const auto& chunk : chunksCache)
				{
					if (chunk.first.layer == 0)        //first layer..
					{
						cascade.nonEmptyChunks.emplace(chunk.first);
						consts.chunkcoord = chunk.first.coord * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;
						consts.objectsCount = chunk.second.modelsCount;
						if (chunk.second.modelsCount > 0)
						{
							memcpy(consts.objects, chunk.second.models, sizeof(uint32_t) * consts.objectsCount);
							Renderer::dispatch(
								renderData.commandBuffer,
								chunkDispatchGroups,
								chunkDispatchGroups,
								chunkDispatchGroups, sdfData.pipelineNoRead.get(), &consts, { sdfData.sets[cascadeIndex] });
						}
						else
						{
							pushConsts.chunkCoord = chunk.first.coord * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;
							//clear..
							Renderer::dispatch(
								renderData.commandBuffer,
								chunkDispatchGroups,
								chunkDispatchGroups,
								chunkDispatchGroups, sdfData.clearPipeline.get(), &pushConsts, { sdfData.clearSets });
						}

						anyChunkDispatch = true;
					}
				}

				for (const auto& chunk : chunksCache)
				{
					//additive layers, combine with existing chunk data
					if (chunk.first.layer != 0 && chunk.second.modelsCount > 0)
					{
						cascade.nonEmptyChunks.emplace(chunk.first);
						consts.objectsCount = chunk.second.modelsCount;
						consts.chunkcoord = chunk.first.coord * CONSTS_SDF_RASTERIZE_CHUNK_SIZE;
						memcpy(consts.objects, chunk.second.models, sizeof(uint32_t) * consts.objectsCount);

						if (chunk.second.modelsCount != 0)
						{
							Renderer::dispatch(
								renderData.commandBuffer,
								chunkDispatchGroups,
								chunkDispatchGroups,
								chunkDispatchGroups, sdfData.pipeline.get(), &consts, { sdfData.sets[cascadeIndex] });

							anyChunkDispatch = true;
						}
					}
				}

				if (anyChunkDispatch || updated)
				{
					const int32_t mipDispatchGroups = math::divideAndRoundUp(resolutionMip, CONSTS_SDF_MIP_GROUP_SIZE);

					GlobalSDFMipmapPushConsts pushConsts{
						sdfResolution,
						CONSTS_SDF_RASTERIZE_MIP_FACTOR,
						cascadeIndex * sdfResolution,
						cascadeIndex * resolutionMip,
						cascadeMaxDistance };

					sdfData.set0->setTexture("uGlobalSDF", sdfPublic.texture);
					sdfData.set0->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
					Renderer::dispatch(renderData.commandBuffer, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups, sdfData.pipelineMip.get(), &pushConsts, { sdfData.set0 });

					if (!chunksCache.empty())
					{
						fillFlood(mipDispatchGroups, cascadeIndex, resolutionMip, pushConsts, sdfData, sdfPublic, renderData);
					}

					sdfPublic.sdfCommonData.cascadeVoxelSize[cascadeIndex] = cascadeVoxelSize;
					sdfPublic.sdfCommonData.cascadePosDistance[cascadeIndex] = {
						center, cascadeDistance };
				}
			}
		}
	}        // namespace merge_CONSTS_sdf

	auto registerGlobalDistanceRenderer(SystemQueue& begin, SystemQueue& render, SystemBuilder::Ptr builder) -> void
	{
		builder->registerGlobalComponent<global::component::GlobalDistanceField>([](auto& field) {
			field.sdfBuffer = StorageBuffer::create(CONSTS_SDF_RASTERIZE_MODEL_SET_MAX_COUNT * sizeof(ObjectRasterizeData), nullptr, { false, MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });
			field.shader = Shader::create("shaders/SDF/SDFRasterizeModel.shader", { {"uMeshSDF", CONSTS_SDF_RASTERIZE_MODEL_SET_MAX_COUNT} });
			field.shaderNoRead = Shader::create("shaders/SDF/SDFRasterizeModelNoRead.shader", { {"uMeshSDF", CONSTS_SDF_RASTERIZE_MODEL_SET_MAX_COUNT} });

			PipelineInfo info;
			//######################################################
			info.pipelineName = "SDFRasterizeModel";
			info.shader = field.shader;
			field.pipeline = Pipeline::get(info);
			//######################################################
			info.shader = field.shaderNoRead;
			info.pipelineName = "SDFRasterizeModelNoRead";
			field.pipelineNoRead = Pipeline::get(info);
			//#####################################################
			for (auto i = 0; i < 4; i++)
			{
				field.sets[i] = DescriptorSet::create({ 0, field.shader.get(), 1, nullptr, CONSTS_SDF_RASTERIZE_MODEL_SET_MAX_COUNT });
			}

			field.shaderMip = Shader::create("shaders/SDF/GlobalSDFMipmap.shader");
			info.pipelineName = "GlobalSDFMipmap";
			info.shader = field.shaderMip;
			field.pipelineMip = Pipeline::get(info);
			field.set0 = DescriptorSet::create({ 0, field.shaderMip.get() });
			field.set1 = DescriptorSet::create({ 0, field.shaderMip.get() });
			field.set2 = DescriptorSet::create({ 0, field.shaderMip.get() });

			//#####################################################

			field.clearShader = Shader::create("shaders/SDF/GlobalSDFClear.shader");
			info.pipelineName = "GlobalSDFClear";
			info.shader = field.clearShader;
			field.clearPipeline = Pipeline::get(info);
			field.clearSets = DescriptorSet::create({ 0, field.clearShader.get() });
			});
		builder->registerGlobalComponent<global::component::GlobalDistanceFieldPublic>();
		builder->registerWithinQueue<generate_sdf::system>(render);
		builder->registerWithinQueue<merge_CONSTS_sdf::system>(render);
		builder->registerWithinQueue<on_scene_changed::system>(begin);
		builder->registerOnImGui<on_imgui::system>();
	}

	auto registerSDFVisualizer(SystemQueue& begin, SystemQueue& render, SystemBuilder::Ptr builder) -> void
	{
		builder->registerGlobalComponent<global::component::SDFVisualizer>([](auto& pipline) {
			pipline.shader = Shader::create("shaders/SDF/MeshSDFDebug.shader");
			pipline.descriptors.emplace_back(DescriptorSet::create({ 0, pipline.shader.get() }));
			});
		builder->registerWithinQueue<draw_voxel::system>(render);
	}
}        // namespace maple::sdf
