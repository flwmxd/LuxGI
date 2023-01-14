//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GlobalSurfaceAtlas.h"
#include "GlobalDistanceField.h"

#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/Mesh.h"
#include "Engine/Renderer/DeferredOffScreenRenderer.h"
#include "Engine/Renderer/RendererData.h"
#include "SurfaceAtlasTile.h"

#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"
#include "RHI/VertexBuffer.h"

#include "Math/BoundingSphere.h"
#include "MeshDistanceField.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

#include "GlobalDistanceField.h"
#include "ImGui/ImGuiHelpers.h"
#include "Math/MathUtils.h"
#include "Math/OrientedBoundingBox.h"
#include "SurfaceAtlasTile.h"
#include <imgui.h>

namespace maple::sdf
{
	namespace
	{
		constexpr int32_t  GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION = 40;
		constexpr int32_t  GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE = 4;
		constexpr int32_t  CHUNK_CACHE_SIZE = GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION;
		constexpr uint16_t GLOBAL_SURFACE_ATLAS_TILE_SIZE_MIN = 32;        // The minimum size of the tile
		constexpr uint16_t GLOBAL_SURFACE_ATLAS_TILE_SIZE_MAX = 128;        // The maximum size of the tile
		constexpr bool     GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES = false;
		constexpr float    GLOBAL_SURFACE_ATLAS_TILE_PADDING = 1.f;
		constexpr float    GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET = 0.1f;
		constexpr int32_t  CULL_OBJECT_FRAME_SIZE = 8;
		constexpr int32_t  GPU_ASYNC_LATENCY = 2;        // Default latency (in frames) between CPU and GPU used for async gpu jobs
		constexpr int32_t  GLOBAL_SURFACE_ATLAS_CULL_LOCAL_SIZE = 32;
		constexpr int32_t  STATIC_REDRWA_FRAMES = 120;
		constexpr int32_t  REDRWA_FRAMES = STATIC_REDRWA_FRAMES;
		constexpr int32_t  GI_FRAMES = 15;

		struct AtlasTileVertex
		{
			glm::vec2 position;
			glm::vec2 tileUV;
			uint32_t  tileAddress;
		};

		struct ObjectBuffer
		{
			glm::vec4  objectBounds;
			uint32_t   tileOffset[6];
			glm::ivec2 padding;
			glm::mat4  transform; 
			glm::vec4  extends;
		};

		struct TileBuffer
		{
			glm::vec4 extends;
			glm::mat4 transform;
			glm::vec4 objectBounds;
		};
	}        // namespace

	namespace surface
	{
		namespace component
		{
			struct LightFrameData
			{
				uint64_t lastFrameUsed = 0;
				uint64_t lastFrameUpdated = 0;
			};
		}        // namespace component

		namespace global::component
		{
			struct GlobalSurface
			{
				uint32_t resolution = 4096;

				bool dirty = true;
				bool allLightingDirty = false;

				VertexBuffer::Ptr            vertexBuffer;
				std::vector<AtlasTileVertex> vertexBufferArray;

				float shadowBias = 1.f;

				std::shared_ptr<SurfaceAtlasTile> surfaceAtlas;

				uint64_t lastFrameAtlasInsertFail = 0;
				uint64_t lastFrameAtlasDefragmentation = 0;

				int32_t            culledObjectsCounterIndex = -1;
				StorageBuffer::Ptr culledObjectsSizeBuffer;
				uint64_t           culledObjectsSizeFrames[CULL_OBJECT_FRAME_SIZE] = {};

				std::unordered_set<entt::entity> dirtyEntities;

				std::vector<entt::entity> cameraCulledObjects;
				std::vector<entt::entity> updatedObjects;

				std::vector<ObjectBuffer> objectBuffer;
				std::vector<TileBuffer>   tileBuffer;

				DescriptorSet::Ptr deferredColorDescriptor;
				uint32_t           objectsBufferCapacity = 0;

				DescriptorSet::Ptr cullingDescriptor;
				DescriptorSet::Ptr copyEmissiveDescriptor;
				DescriptorSet::Ptr deferredLightDescriptor;
				DescriptorSet::Ptr indirectLightDescriptor;

				uint32_t lastGIUpdated = 0;
			};
		}        // namespace global::component

		namespace basic_update
		{
			inline auto createTexture(const std::string& name, TextureFormat foramt, uint32_t resolution)
			{
				auto texture = Texture2D::create();
				texture->setName(name);
				texture->buildTexture(foramt, resolution, resolution);
				return texture;
			}


			inline auto cacheToSurface(
				entt::entity entity,
				maple::component::MeshRenderer& render,
				maple::component::Transform& transform,
				sdf::component::MeshDistanceField& sdf,
				component::MeshSurfaceAtlas& atlas,
				const BoundingSphere& boundingSphere,
				const BoundingBox& objectBounds,
				global::component::GlobalSurface& surface,
				const maple::global::component::Profiler& profiler,
				const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				float distance, float radius)
			{
				OrientedBoundingBox objectObb(objectBounds);

				atlas.obb = OrientedBoundingBox(sdf.aabb);
				atlas.obb.setTransform(transform.getWorldMatrix());
				atlas.radius = radius;

				const float tilesScale = 1.f / sdfPublic.gloalScale;

				bool anyTile = false;
				bool dirty = false;

				for (int32_t tileIndex = 0; tileIndex < 6; tileIndex++)
				{
					auto boundsSizeTile = atlas.obb.getExtends() * 2.f * transform.getWorldScale();
					auto abs = glm::abs(boundsSizeTile);

					uint16_t tileResolution = (uint16_t)(math::max3(abs) * tilesScale);
					if (tileResolution < 4)
					{
						if (atlas.tiles[tileIndex])
						{
							atlas.tiles[tileIndex]->free();
							atlas.tiles[tileIndex] = nullptr;
						}
						continue;
					}

					tileResolution = std::clamp(tileResolution, GLOBAL_SURFACE_ATLAS_TILE_SIZE_MIN, GLOBAL_SURFACE_ATLAS_TILE_SIZE_MAX);
					tileResolution = math::alignDown<uint16_t>(tileResolution, 8);
					if (atlas.tiles[tileIndex])
					{
						constexpr uint16_t tileRefitResolutionStep = 32;
						const uint16_t     currentSize = atlas.tiles[tileIndex]->width;
						if (std::abs(tileResolution - currentSize) < tileRefitResolutionStep)
						{
							anyTile = true;
							continue;
						}
						atlas.tiles[tileIndex]->free();
					}

					auto tile = surface.surfaceAtlas->insert(tileResolution, tileResolution, 0, atlas, tileIndex);
					if (tile)
					{
						atlas.tiles[tileIndex] = tile;
						anyTile = true;
						dirty = true;
					}
					else
					{
						atlas.tiles[tileIndex] = nullptr;
						surface.lastFrameAtlasInsertFail = profiler.frameCount;
					}
				}

				if (!anyTile)
					return;

				uint32_t redrawFramesCount = REDRWA_FRAMES;

				if ((profiler.frameCount - atlas.lastFrameUpdated) >= redrawFramesCount)
					dirty = true;

				atlas.lastFrameUsed = profiler.frameCount;

				if (dirty || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES)
				{
					atlas.lastFrameUpdated = profiler.frameCount;
					atlas.lightingUpdateFrame = profiler.frameCount;
					surface.dirtyEntities.emplace(entity);
				}

				auto  objectAddress = surface.objectBuffer.size();
				auto& objectBuffer = surface.objectBuffer.emplace_back();

				MAPLE_ASSERT(boundingSphere.radius > 0, "boundingSphere.radius should be greater than zero");

				objectBuffer.objectBounds = glm::vec4(boundingSphere.center, boundingSphere.radius);
				objectBuffer.transform = atlas.obb.getTransform();
				objectBuffer.extends = { atlas.obb.getExtends() * transform.getWorldScale(), 1.f };        //should use sdf aabb to interects.
				memset(objectBuffer.tileOffset, 0, sizeof(uint32_t) * 6);

				for (int32_t tileIndex = 0; tileIndex < 6; tileIndex++)
				{
					auto tile = atlas.tiles[tileIndex];
					if (!tile)
						continue;

					tile->objectAddressOffset = objectAddress;
					tile->address = surface.tileBuffer.size();
					objectBuffer.tileOffset[tileIndex] = tile->address;

					glm::vec3 xAxis(0.f);
					glm::vec3 yAxis(0.f);
					glm::vec3 zAxis(0.f);
					zAxis[tileIndex / 2] = tileIndex & 1 ? 1.0f : -1.0f;        // 1 3 5
					yAxis = tileIndex == 2 || tileIndex == 3 ? maple::RIGHT : maple::UP;
					xAxis = glm::cross(yAxis, zAxis);

					//use object aabb to create view matrix
					auto      extends = glm::max(objectObb.getExtends() * transform.getWorldScale(), glm::vec3(0.05));
					glm::vec3 localSpaceOffset = -zAxis * extends;

					xAxis = glm::normalize(xAxis);
					yAxis = glm::normalize(yAxis);
					zAxis = glm::normalize(zAxis);

					tile->viewPosition = objectObb.getTransform() * glm::vec4(localSpaceOffset, 1.f);        //translate to center

					tile->viewDirection = zAxis;
					tile->viewUp = yAxis;
					tile->viewMatrix = glm::lookAt(tile->viewPosition, tile->viewPosition - localSpaceOffset, yAxis);
					//tile->viewMatrix[3] = {0, 0, 0, 1};

					auto viewExtent = maple::component::Transform::localToWorldVector(tile->viewMatrix, extends);        //obj bounds
					tile->viewBoundsSize = glm::abs(viewExtent) * 2.0f;

					// Per-tile data
					const float tileWidth = (float)tile->width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
					const float tileHeight = (float)tile->height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
					auto& tileData = surface.tileBuffer.emplace_back();

					tileData.objectBounds = glm::vec4(tile->viewBoundsSize, 0.0f);
					tileData.transform = tile->viewMatrix;
					tileData.transform[3] = { 0, 0, 0, 1 };
					tileData.extends = glm::vec4(tile->x, tile->y, tileWidth, tileHeight) / (float)surface.resolution;
				}
			}


			/**
			 * estimate Object Size from the culled data.
			 * the default is surface.atlasObjects.size() * 1.3f
			 * read the culled data size from GPU, dispatch them into different frame to detect dynamic size.
			 */
			inline auto estimateObjectSize(
				global::component::GlobalSurface& surface,
				const maple::global::component::Profiler& profiler,
				const maple::component::RendererData& renderData,
				const maple::global::component::RenderDevice& renderDevice,
				surface::global::component::GlobalSurfacePublic& surfacePublic)
			{
				if (!surface.cameraCulledObjects.empty())
				{
					surface.objectsBufferCapacity = (uint32_t)(surface.cameraCulledObjects.size() * 1.3f);
					if (surfacePublic.chunkBuffer)
					{
						if (!surface.culledObjectsSizeBuffer)
						{        //StagingReadback
							surface.culledObjectsSizeBuffer = StorageBuffer::create(CULL_OBJECT_FRAME_SIZE * sizeof(uint32_t), nullptr, { false, MemoryUsage::MEMORY_USAGE_GPU_TO_CPU });
						}

						if (surface.culledObjectsCounterIndex != -1)
						{
							// Get the last counter value
							auto data = (uint32_t*)surface.culledObjectsSizeBuffer->map();
							if (data)
							{
								uint32_t counter = data[surface.culledObjectsCounterIndex];
								surface.culledObjectsSizeBuffer->unmap();
								if (counter > 0)
								{
									surface.objectsBufferCapacity = counter;
								}
							}
						}
						else        //cull -> is negative
						{
							for (int32_t i = 0; i < CULL_OBJECT_FRAME_SIZE; i++)
							{
								if (profiler.frameCount - surface.culledObjectsSizeFrames[i] > GPU_ASYNC_LATENCY)
								{
									surface.culledObjectsCounterIndex = i;
									break;
								}
							}
						}

						if (surface.culledObjectsCounterIndex != -1 && surfacePublic.culledObjectsBuffer != nullptr)
						{
							surface.culledObjectsSizeFrames[surface.culledObjectsCounterIndex] = profiler.frameCount;
							renderDevice.device->copyBuffer(renderData.commandBuffer,
								surfacePublic.culledObjectsBuffer,
								surface.culledObjectsSizeBuffer,
								sizeof(uint32_t), surface.culledObjectsCounterIndex * sizeof(uint32_t), 0);
						}
					}

					surface.objectsBufferCapacity = std::min(math::alignUp<uint32_t>(surface.objectsBufferCapacity * sizeof(uint32_t), 4096u), std::numeric_limits<uint32_t>::max());

					if (!surfacePublic.culledObjectsBuffer)
						surfacePublic.culledObjectsBuffer = StorageBuffer::create(surface.objectsBufferCapacity, nullptr, { false, MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });

					if (surfacePublic.culledObjectsBuffer->getSize() < surface.objectsBufferCapacity)
					{
						surfacePublic.culledObjectsBuffer->resize(surface.objectsBufferCapacity);
					}
				}
				else
				{
					surface.objectsBufferCapacity = 0;
				}
			}

			inline auto system(ioc::Registry registry, global::component::GlobalSurface& surface,
				const deferred::global::component::DeferredData& deferredData,
				const maple::global::component::Profiler& profiler,
				const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const maple::component::CameraView& cameraView,
				const maple::global::component::RenderDevice& renderDevice,
				const maple::component::RendererData& renderData,
				surface::global::component::GlobalSurfacePublic& surfacePublic)
			{
				auto group = registry.getRegistry().view<
					maple::component::MeshRenderer,
					maple::component::Transform,
					sdf::component::MeshDistanceField>();

				if (group.begin() == group.end())
					return;

				surface.dirtyEntities.clear();
				surface.objectBuffer.clear();
				surface.tileBuffer.clear();
				surface.cameraCulledObjects.clear();

				if (surface.deferredColorDescriptor == nullptr)
				{
					surface.deferredColorDescriptor = DescriptorSet::create({ 0, Shader::create("shaders/SDF/SDFDeferredColor.shader").get() });
				}

				if (surface.cullingDescriptor == nullptr)
				{
					surface.cullingDescriptor = DescriptorSet::create({ 0, Shader::create("shaders/SDF/SDFCulling.shader").get() });
				}

				if (surface.copyEmissiveDescriptor == nullptr)
				{
					surface.copyEmissiveDescriptor = DescriptorSet::create({ 0, Shader::create("shaders/SDF/CopyEmissive.shader").get() });
				}

				if (surface.deferredLightDescriptor == nullptr)
				{
					surface.deferredLightDescriptor = DescriptorSet::create({ 0, Shader::create("shaders/SDF/SDFDeferredLight.shader").get() });
				}

				if (surface.indirectLightDescriptor == nullptr)
				{
					surface.indirectLightDescriptor = DescriptorSet::create({ 0, Shader::create("shaders/SDF/SDFAtlasIndirectLight.shader").get() });
				}

				if (surface.dirty)
				{        //TODO should compress like lumen...
					surfacePublic.surfaceGBuffer0 = createTexture("SurfaceGBuffer-Color", TextureFormat::RGBA16, surface.resolution);
					surfacePublic.surfaceGBuffer1 = createTexture("SurfaceGBuffer-Normal", TextureFormat::RGBA16, surface.resolution);
					surfacePublic.surfaceGBuffer2 = createTexture("SurfaceGBufferPRB", TextureFormat::RGBA8, surface.resolution);
					surfacePublic.surfaceEmissive = createTexture("SurfaceEmissive", TextureFormat::R11G11B10, surface.resolution);
					surfacePublic.surfaceLightCache = createTexture("SurfaceLightCache", TextureFormat::R11G11B10, surface.resolution);
					surfacePublic.surfaceDepth = TextureDepth::create(surface.resolution, surface.resolution);

					if (surfacePublic.chunkBuffer == nullptr)
					{
						surfacePublic.chunkBuffer = StorageBuffer::create(sizeof(int32_t) * CHUNK_CACHE_SIZE,
							nullptr,
							BufferOptions{ true, MemoryUsage::MEMORY_USAGE_GPU_ONLY });
					}
				}
				else
				{
					if (profiler.frameCount - surface.lastFrameAtlasInsertFail < 10 &&
						profiler.frameCount - surface.lastFrameAtlasDefragmentation > 60)
					{
						return;
					}
				}

				if (surface.vertexBuffer == nullptr)
					surface.vertexBuffer = VertexBuffer::create(BufferUsage::Dynamic);

				if (surface.surfaceAtlas == nullptr)
					surface.surfaceAtlas = std::make_shared<SurfaceAtlasTile>(0, 0, surface.resolution, surface.resolution);

				const float minObjectRadius = sdfPublic.minObjectRadius * sdfPublic.gloalScale;
				const float distance = cameraView.farPlane;        //TODO... I should render object near 200 meter like Lumen or GI distance?

				std::vector<entt::entity> deleteQueue;

				auto addToDelete = [&](const maple::global::component::Profiler& profiler, component::MeshSurfaceAtlas& atlas, entt::entity entity) {
					if (profiler.frameCount != atlas.lastFrameUsed)
					{
						deleteQueue.emplace_back(entity);
						for (auto& tile : atlas.tiles)
						{
							if (tile)
							{
								tile->free();
							}
						}
					}
					else
					{
						surface.cameraCulledObjects.emplace_back(entity);
					}
				};

				for (auto obj : group.each())
				{
					auto [entity, render, transform, sdf] = obj;
					BoundingBox    objectBounds = sdf.aabb.transform(transform.getWorldMatrix());
					BoundingSphere sphereBox = objectBounds;
					auto           objToView = glm::distance(objectBounds.center(), cameraView.cameraTransform->getWorldPosition());

					if (sphereBox.radius >= minObjectRadius && objToView < distance)
					{
						auto& atlas = registry.getRegistry().get_or_emplace<component::MeshSurfaceAtlas>(entity);

						cacheToSurface(entity, render, transform, sdf, atlas, sphereBox, *render.mesh->getBoundingBox(), surface, profiler, sdfPublic, objToView, sphereBox.radius);
						addToDelete(profiler, atlas, entity);
					}
				}

				for (auto ent : deleteQueue)
				{
					registry.removeComponent<component::MeshSurfaceAtlas>(ent);        //now it is immediate mode, but it would be a delay in the future.
				}

				estimateObjectSize(surface, profiler, renderData, renderDevice, surfacePublic);
			}
		}        // namespace basic_update

		namespace on_imgui
		{
			inline auto system(global::component::GlobalSurface& surface,
				global::component::GlobalSurfacePublic& surfacePublic)
			{
				if (ImGui::Begin("Surface Atlas Debugger."))
				{
					ImGui::Columns(2);
					ImGuiHelper::property("Shadow Bias(Lighting Cache)", surface.shadowBias, 0.f, 15.f);

					ImGuiHelper::showProperty("ObjectsBufferCapacity", std::to_string(surface.objectsBufferCapacity));

					ImGui::Columns(1);

					if (ImGui::CollapsingHeader("SurfaceGBuffer0") && surfacePublic.surfaceGBuffer0)
					{
						ImGuiHelper::image(surfacePublic.surfaceGBuffer0.get(), { 512, 512 });
					}

					if (ImGui::CollapsingHeader("SurfaceGBuffer1") && surfacePublic.surfaceGBuffer1)
					{
						ImGuiHelper::image(surfacePublic.surfaceGBuffer1.get(), { 512, 512 });
					}

					if (ImGui::CollapsingHeader("SurfaceGBuffer2") && surfacePublic.surfaceGBuffer2)
					{
						ImGuiHelper::image(surfacePublic.surfaceGBuffer2.get(), { 512, 512 });
					}

					if (ImGui::CollapsingHeader("SurfaceEmissive") && surfacePublic.surfaceEmissive)
					{
						ImGuiHelper::image(surfacePublic.surfaceEmissive.get(), { 512, 512 });
					}

					if (ImGui::CollapsingHeader("SurfaceLightCache") && surfacePublic.surfaceLightCache)
					{
						ImGuiHelper::image(surfacePublic.surfaceLightCache.get(), { 512, 512 });
					}

					if (ImGui::CollapsingHeader("SurfaceDepth") && surfacePublic.surfaceDepth)
					{
						ImGuiHelper::image(surfacePublic.surfaceDepth.get(), { 512, 512 });
					}
				}
				ImGui::End();
			}
		}        // namespace on_imgui

		namespace on_render
		{

			inline auto addTiles(global::component::GlobalSurface& surface, SurfaceAtlasTile* tile)
			{
				const glm::vec2 posToClipMul(2.0f / surface.resolution, 2.0f / surface.resolution);
				const glm::vec2 posToClipAdd(-1.0f, -1.0f);

				glm::vec2 minPos((float)tile->x, (float)tile->y);
				glm::vec2 maxPos((float)(tile->x + tile->width), (float)(tile->y + tile->height));

				auto min = minPos * posToClipMul + posToClipAdd;
				auto max = maxPos * posToClipMul + posToClipAdd;

				surface.vertexBufferArray.emplace_back().position = max;
				surface.vertexBufferArray.emplace_back().position = { min.x, max.y };
				surface.vertexBufferArray.emplace_back().position = min;
				surface.vertexBufferArray.emplace_back().position = min;
				surface.vertexBufferArray.emplace_back().position = { max.x, min.y };
				surface.vertexBufferArray.emplace_back().position = max;
			}

			inline auto addTiles2(global::component::GlobalSurface& surface, SurfaceAtlasTile* tile)
			{
				const glm::vec2 posToClipMul(2.0f / surface.resolution, 2.0f / surface.resolution);
				const glm::vec2 posToClipAdd(-1.0f, -1.0f);

				glm::vec2 minPos((float)tile->x, (float)tile->y);
				glm::vec2 maxPos((float)(tile->x + tile->width), (float)(tile->y + tile->height));

				auto min = minPos * posToClipMul + posToClipAdd;
				auto max = maxPos * posToClipMul + posToClipAdd;

				glm::vec2 minUV(0, 0);
				glm::vec2 maxUV(1, 1);

				surface.vertexBufferArray.push_back({ max, maxUV, tile->address });
				surface.vertexBufferArray.push_back({ {min.x, max.y}, {minUV.x, maxUV.y}, tile->address });
				surface.vertexBufferArray.push_back({ min, minUV, tile->address });
				surface.vertexBufferArray.push_back({ min, minUV, tile->address });
				surface.vertexBufferArray.push_back({ {max.x, min.y}, {maxUV.x, minUV.y}, tile->address });
				surface.vertexBufferArray.push_back({ max, maxUV, tile->address });
			}

			inline auto flush(const global::component::GlobalSurface& surface, global::component::GlobalSurfacePublic& surfacePublic)
			{
				if (!surface.objectBuffer.empty())
				{
					if (surfacePublic.ssboObjectBuffer == nullptr)
					{
						surfacePublic.ssboObjectBuffer = StorageBuffer::create({ false, MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });
					}

					if (surfacePublic.ssboObjectBuffer->getSize() < surface.objectBuffer.size() * sizeof(ObjectBuffer))
					{
						surfacePublic.ssboObjectBuffer->resize(surface.objectBuffer.size() * sizeof(ObjectBuffer));
					}
					surfacePublic.ssboObjectBuffer->setData(surface.objectBuffer.size() * sizeof(ObjectBuffer), surface.objectBuffer.data());
				}

				if (!surface.tileBuffer.empty())
				{
					if (surfacePublic.ssboTileBuffer == nullptr)
					{
						surfacePublic.ssboTileBuffer = StorageBuffer::create({ false, MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });
					}

					if (surfacePublic.ssboTileBuffer->getSize() < surface.tileBuffer.size() * sizeof(TileBuffer))
					{
						surfacePublic.ssboTileBuffer->resize(surface.tileBuffer.size() * sizeof(TileBuffer));
					}
					surfacePublic.ssboTileBuffer->setData(surface.tileBuffer.size() * sizeof(TileBuffer), surface.tileBuffer.data());
				}
			}

			inline auto culling(ioc::Registry registry, global::component::GlobalSurface& surface,
				sdf::global::component::GlobalDistanceFieldPublic& field,
				const maple::component::CameraView& cameraView,
				const maple::component::RendererData& renderData,
				global::component::GlobalSurfacePublic& surfacePublic, float giDistance)
			{
				if (surface.objectsBufferCapacity != 0)
				{
					flush(surface, surfacePublic);
					// Clear chunks counter (uint at 0 is used for a counter)
					const uint32_t counter = 1;
					surfacePublic.culledObjectsBuffer->setData(sizeof(uint32_t), &counter);

					if (surfacePublic.ssboObjectBuffer == nullptr)
						return;

					surface.cullingDescriptor->setStorageBuffer("SDFObjectBuffer", surfacePublic.ssboObjectBuffer);
					surface.cullingDescriptor->setStorageBuffer("SDFAtlasChunkBuffer", surfacePublic.chunkBuffer);
					surface.cullingDescriptor->setStorageBuffer("SDFCullObjectBuffer", surfacePublic.culledObjectsBuffer);

					// Cull objects into chunks (1 thread per chunk)
					field.globalSurfaceAtlasData.cameraPos = cameraView.cameraTransform->getWorldPosition();
					field.globalSurfaceAtlasData.culledObjectsCapacity = surface.objectsBufferCapacity;
					field.globalSurfaceAtlasData.resolution = surface.resolution;
					field.globalSurfaceAtlasData.chunkSize = giDistance / GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION;
					field.globalSurfaceAtlasData.objectsCount = surface.objectBuffer.size();

					const int32_t chunkDispatchGroups = GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION / GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE;

					PipelineInfo pipelineInfo;
					pipelineInfo.shader = Shader::create("shaders/SDF/SDFCulling.shader");
					pipelineInfo.pipelineName = "SDFCulling";
					auto pipeline = Pipeline::get(pipelineInfo);
					Renderer::dispatch(renderData.commandBuffer, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups, pipeline.get(), &field.globalSurfaceAtlasData, { surface.cullingDescriptor });
				}
			}

			inline auto deferredColor(ioc::Registry registry, const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const maple::component::CameraView& cameraView,
				global::component::GlobalSurface& surface,
				maple::global::component::RenderDevice& renderDevice,
				const maple::component::RendererData& renderData,
				deferred::global::component::DeferredData& deferredData,
				global::component::GlobalSurfacePublic& surfacePublic)
			{
				auto group = registry.getRegistry().view<
					maple::component::MeshRenderer,
					maple::component::Transform,
					sdf::component::MeshDistanceField,
					sdf::surface::component::MeshSurfaceAtlas>();

				const float minObjectRadius = sdfPublic.minObjectRadius * sdfPublic.gloalScale;
				const float distance = cameraView.farPlane;        //TODO... I should render object near 200 meter like Lumen or GI distance?

				PipelineInfo info;
				info.polygonMode = PolygonMode::Fill;
				info.depthFunc = StencilType::Always;
				info.swapChainTarget = false;
				info.clearTargets = false;
				info.depthTest = false;
				info.transparencyEnabled = false;
				info.colorTargets[0] = surfacePublic.surfaceGBuffer0;
				info.colorTargets[1] = surfacePublic.surfaceGBuffer1;
				info.colorTargets[2] = surfacePublic.surfaceGBuffer2;
				info.colorTargets[3] = surfacePublic.surfaceEmissive;
				info.depthTarget = surfacePublic.surfaceDepth;

				if (!surface.dirtyEntities.empty())
				{
					//surface.lastFrameDirtyEntities = surface.dirtyEntities;

					if (surface.dirty || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES)
					{
						renderDevice.device->clearRenderTarget(surfacePublic.surfaceGBuffer0, renderData.commandBuffer, glm::vec4(0));
						renderDevice.device->clearRenderTarget(surfacePublic.surfaceGBuffer1, renderData.commandBuffer, glm::vec4(0));
						renderDevice.device->clearRenderTarget(surfacePublic.surfaceGBuffer2, renderData.commandBuffer, glm::vec4(0));
						renderDevice.device->clearRenderTarget(surfacePublic.surfaceEmissive, renderData.commandBuffer, glm::vec4(0));
						renderDevice.device->clearRenderTarget(surfacePublic.surfaceDepth, renderData.commandBuffer, glm::vec4(1.f));
					}
					else
					{        //clear tile by tile
						surface.vertexBufferArray.clear();
						surface.vertexBufferArray.reserve(surface.dirtyEntities.size() * 6);        //6 faces..

						info.depthTest = true;

						for (auto entity : surface.dirtyEntities)
						{
							auto [_1, _2, _3, atlas] = group.get(entity);
							for (auto tileIndex = 0; tileIndex < 6; tileIndex++)
							{
								auto tile = atlas.tiles[tileIndex];
								if (tile == nullptr)
									continue;
								addTiles(surface, tile);
							}
						}
						//actually in here, it is unnecessary to clear them because we only need to mark them in CPU side.
						//but if they are cleared, we can debug them more easily.
						surface.vertexBuffer->setData(surface.vertexBufferArray.size() * sizeof(AtlasTileVertex), surface.vertexBufferArray.data());
						info.pipelineName = "Clear-Tile";
						info.shader = Shader::create("shaders/SDF/SurfaceClear.shader");
						auto pipeline = Pipeline::get(info);
						pipeline->bind(renderData.commandBuffer);
						surface.vertexBuffer->bind(renderData.commandBuffer, pipeline.get());
						renderDevice.device->drawInstanced(renderData.commandBuffer, surface.vertexBufferArray.size(), 1);
						surface.vertexBuffer->unbind();
						pipeline->end(renderData.commandBuffer);
					}

					info.colorTargets[4] = nullptr;

					info.shader = Shader::create("shaders/SDF/SDFDeferredColor.shader");

					std::shared_ptr<Pipeline> pipeline;

					for (auto entity : surface.dirtyEntities)
					{
						//execute GBuffer pass from obb all sides...
						auto [mesh, transform, sdf, atlas] = group.get(entity);
						for (auto tileIndex = 0; tileIndex < 6; tileIndex++)
						{
							auto tile = atlas.tiles[tileIndex];
							if (!tile)
								continue;
							const float tileWidth = (float)tile->width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
							const float tileHeight = (float)tile->height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;

							uint32_t start = 0;

							for (auto i = 0; i < mesh.mesh->getSubMeshIndex().size(); i++)
							{
								auto halfW = tile->viewBoundsSize.x / 2;
								auto halfH = tile->viewBoundsSize.y / 2;

								auto proj = glm::ortho(-halfW, halfW,
									-halfH, halfH,
									-GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET,
									tile->viewBoundsSize.z + 2 * GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET);

								auto viewProj = proj * tile->viewMatrix * glm::scale(glm::mat4(1), transform.getWorldScale());

								auto material = mesh.mesh->getSubMeshIndex().size() > mesh.mesh->getMaterial().size() ?
									deferredData.defaultMaterial.get() :
									mesh.mesh->getMaterial()[i].get();

								//auto prevShader = material->getShader();

								//material->setShader(info.shader);
								material->bind(renderData.commandBuffer);
								auto count = mesh.mesh->getSubMeshIndex()[i] - start;
								info.cullMode = CullMode::Back;
								info.transparencyEnabled = false;
								info.depthTarget = surfacePublic.surfaceDepth;
								info.depthFunc = StencilType::Less;
								info.blendMode = BlendMode::None;
								info.depthTest = true;
								info.transparencyEnabled = false;

								pipeline = Pipeline::get(info);
								pipeline->bind(renderData.commandBuffer, { tile->x, tile->y, tileWidth, tileHeight });

								auto& pushConstants = pipeline->getShader()->getPushConstants()[0];
								pushConstants.setValue("transform", glm::value_ptr(transform.getWorldMatrix()));
								pushConstants.setValue("projView", glm::value_ptr(viewProj));
								pipeline->getShader()->bindPushConstants(renderData.commandBuffer, pipeline.get());

								mesh.mesh->getVertexBuffer()->bind(renderData.commandBuffer, pipeline.get());
								mesh.mesh->getIndexBuffer()->bind(renderData.commandBuffer);
								Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { surface.deferredColorDescriptor, material->getDescriptorSet() });
								Renderer::drawIndexed(renderData.commandBuffer, DrawType::Triangle, count, start);
								pipeline->end(renderData.commandBuffer);
								mesh.mesh->getVertexBuffer()->unbind();
								mesh.mesh->getIndexBuffer()->unbind();
								start = mesh.mesh->getSubMeshIndex()[i];
								//material->setShader(prevShader);
							}
						}
					}
				}
			}

			inline auto deferredLighting(ioc::Registry registry, const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const maple::component::CameraView& cameraView,
				global::component::GlobalSurface& surface,
				maple::global::component::RenderDevice& renderDevice,
				const maple::component::RendererData& renderData,
				const maple::global::component::Profiler& profiler,
				global::component::GlobalSurfacePublic& surfacePublic)
			{
				if (surfacePublic.ssboTileBuffer == nullptr)
					return false;

				auto group = registry.getRegistry().view<
					maple::component::MeshRenderer,
					maple::component::Transform,
					sdf::component::MeshDistanceField,
					sdf::surface::component::MeshSurfaceAtlas>();

				if (!surface.cameraCulledObjects.empty())
				{
					surface.updatedObjects.clear();
					surface.copyEmissiveDescriptor->setTexture("uEmissiveSampler", surfacePublic.surfaceEmissive);
					surface.copyEmissiveDescriptor->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);
					surface.copyEmissiveDescriptor->update(renderData.commandBuffer);

					surface.deferredLightDescriptor->setTexture("uColorSampler", surfacePublic.surfaceGBuffer0);
					surface.deferredLightDescriptor->setTexture("uNormalSampler", surfacePublic.surfaceGBuffer1);
					surface.deferredLightDescriptor->setTexture("uPBRSampler", surfacePublic.surfaceGBuffer2);
					surface.deferredLightDescriptor->setTexture("uDepthSampler", surfacePublic.surfaceDepth);
					surface.deferredLightDescriptor->setTexture("uEmissiveSampler", surfacePublic.surfaceEmissive);
					surface.deferredLightDescriptor->setTexture("uGlobalMipSDF", sdfPublic.mipTexture);
					surface.deferredLightDescriptor->setTexture("uGlobalSDF", sdfPublic.texture);
					surface.deferredLightDescriptor->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);

					bool allDirty = false;
					//std::vector<entt::entity> updatedObject;

					//if the light is directional, all object should be redraw...
					auto lightGroup = registry.getRegistry().view<maple::component::Light, component::LightFrameData>();

					for (auto [_, light, frameData] : lightGroup.each())
					{
						frameData.lastFrameUsed = profiler.frameCount;
						if (profiler.frameCount - frameData.lastFrameUpdated < REDRWA_FRAMES)        // todo, if the light is static, it should be more.
							continue;

						if (maple::component::isDirectionalLight(light))
						{
							allDirty = true;
							surface.updatedObjects = surface.cameraCulledObjects;
							break;
						}
					}

					if (profiler.frameCount - surface.lastGIUpdated > GI_FRAMES)
					{
						surface.lastGIUpdated = profiler.frameCount;
						allDirty = true;
						surface.updatedObjects = surface.cameraCulledObjects;
					}

					if (!allDirty)
					{
						for (auto [_, light, frameData] : lightGroup.each())
						{
							frameData.lastFrameUsed = profiler.frameCount;
							if (profiler.frameCount - frameData.lastFrameUpdated < REDRWA_FRAMES)        // todo, if the light is static, it should be more.
								continue;

							if (maple::component::isPointLight(light) || maple::component::isSpotLight(light))
							{
								frameData.lastFrameUpdated = profiler.frameCount;
								for (auto entity : surface.cameraCulledObjects)
								{
									auto [mesh, transform, sdf, atlas] = group.get(entity);
									auto center = sdf.aabb.transform(transform.getWorldMatrix()).center();
									if (glm::distance(center, glm::vec3(light.lightData.position)) < atlas.radius + light.lightData.radius)
									{
										surface.updatedObjects.emplace_back(entity);
									}
								}
							}
						}
					}

					surface.vertexBufferArray.clear();
					for (auto entity : surface.updatedObjects)
					{
						auto [mesh, transform, sdf, atlas] = group.get(entity);
						for (auto tileIndex = 0; tileIndex < 6; tileIndex++)
						{
							auto tile = atlas.tiles[tileIndex];
							if (tile == nullptr)
								continue;
							addTiles2(surface, tile);
						}
					}

					//copy emissive...
					if (!surface.vertexBufferArray.empty())
					{
						PipelineInfo info;
						info.polygonMode = PolygonMode::Fill;
						info.blendMode = BlendMode::None;
						info.swapChainTarget = false;
						info.clearTargets = false;
						info.depthFunc = StencilType::Never;
						info.transparencyEnabled = false;
						info.depthTest = false;
						info.colorTargets[0] = surfacePublic.surfaceLightCache;
						info.pipelineName = "CopyEmissive";

						surface.vertexBuffer->setData(surface.vertexBufferArray.size() * sizeof(AtlasTileVertex), surface.vertexBufferArray.data());
						info.shader = Shader::create("shaders/SDF/CopyEmissive.shader");
						auto pipeline = Pipeline::get(info);
						pipeline->bind(renderData.commandBuffer);
						surface.vertexBuffer->bind(renderData.commandBuffer, pipeline.get());
						Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { surface.copyEmissiveDescriptor });
						renderDevice.device->drawInstanced(renderData.commandBuffer, surface.vertexBufferArray.size(), 1);
						surface.vertexBuffer->unbind();
						pipeline->end(renderData.commandBuffer);
					}
					//lighting....

					for (auto [_, light, frame] : lightGroup.each())
					{
						surface.vertexBufferArray.clear();
						for (auto entity : surface.updatedObjects)
						{
							auto [mesh, transform, sdf, atlas] = group.get(entity);

							auto center = sdf.aabb.transform(transform.getWorldMatrix()).center();

							if (maple::component::isPointLight(light) || maple::component::isSpotLight(light))
							{
								if (glm::distance(center, glm::vec3(light.lightData.position)) > (atlas.radius + light.lightData.radius))
									continue;
							}

							for (auto tileIndex = 0; tileIndex < 6; tileIndex++)
							{
								auto tile = atlas.tiles[tileIndex];
								if (tile == nullptr)
									continue;

								if (maple::component::isDirectionalLight(light))
								{
									//bug..here
									if (glm::dot(tile->viewDirection, glm::vec3(light.lightData.direction)) < 1e-6f)        //parallel
										continue;
								}
								addTiles2(surface, tile);
							}
						}

						if (surface.vertexBufferArray.empty())
							continue;

						surface.deferredLightDescriptor->setUniform("UniformBufferObject", "cameraPos", glm::value_ptr(glm::vec4(cameraView.cameraTransform->getWorldPosition(), surface.shadowBias)), true);
						surface.deferredLightDescriptor->setUniform("UniformBufferObject", "light", &light.lightData, true);
						surface.deferredLightDescriptor->setUniform("UniformBufferObject", "data", &sdfPublic.sdfCommonData, true);
						surface.deferredLightDescriptor->update(renderData.commandBuffer);

						PipelineInfo info;
						info.polygonMode = PolygonMode::Fill;
						info.cullMode = CullMode::None;
						info.blendMode = BlendMode::Add;        //add mode ..
						info.swapChainTarget = false;
						info.clearTargets = false;
						info.depthFunc = StencilType::Never;
						info.transparencyEnabled = true;
						info.depthTest = false;
						info.colorTargets[0] = surfacePublic.surfaceLightCache;
						info.pipelineName = "SDFDeferredLight";

						surface.vertexBuffer->setData(surface.vertexBufferArray.size() * sizeof(AtlasTileVertex), surface.vertexBufferArray.data());
						info.shader = Shader::create("shaders/SDF/SDFDeferredLight.shader");
						auto pipeline = Pipeline::get(info);
						pipeline->bind(renderData.commandBuffer);
						Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { surface.deferredLightDescriptor });
						surface.vertexBuffer->bind(renderData.commandBuffer, pipeline.get());
						renderDevice.device->drawInstanced(renderData.commandBuffer, surface.vertexBufferArray.size(), 1);
						surface.vertexBuffer->unbind();
						pipeline->end(renderData.commandBuffer);
					}
					return allDirty;
				}
				return false;
			}

			inline auto system(ioc::Registry registry, global::component::GlobalSurface& surface,
				const maple::global::component::Profiler& profiler,
				const sdf::global::component::GlobalDistanceFieldPublic& sdfPublic,
				const maple::component::RendererData& renderData,
				maple::global::component::RenderDevice& renderDevice,
				deferred::global::component::DeferredData& deferredData,
				sdf::global::component::GlobalDistanceFieldPublic& field,
				surface::global::component::GlobalSurfacePublic& surfacePublic,
				const maple::component::CameraView& cameraView)
			{
				auto group = registry.getRegistry().view<
					maple::component::MeshRenderer,
					maple::component::Transform,
					sdf::component::MeshDistanceField,
					sdf::surface::component::MeshSurfaceAtlas>();

				if (group.begin() == group.end())
					return;

				deferredColor(registry, sdfPublic, cameraView, surface, renderDevice, renderData, deferredData, surfacePublic);
				culling(registry, surface, field, cameraView, renderData, surfacePublic, sdfPublic.giDistance * sdfPublic.gloalScale);
				surface.allLightingDirty = deferredLighting(registry, sdfPublic, cameraView, surface, renderDevice, renderData, profiler, surfacePublic);
				surface.dirty = false;
			}
		}        // namespace on_render

		namespace indirect_light
		{
			inline auto system(ioc::Registry registry, global::component::GlobalSurface& surface,
				const surface::global::component::GlobalSurfacePublic& surfacePublic,
				const maple::global::component::Profiler& profiler,
				const maple::component::CameraView& cameraView,
				const maple::component::RendererData& renderData,
				maple::global::component::RenderDevice& renderDevice)
			{

				for (auto [_, volume, uniform] : registry.getRegistry()
					.view<ddgi::component::IrradianceVolume,
					ddgi::component::DDGIUniform>().each())
				{

					if (volume.currentIrrdance == nullptr || surfacePublic.ssboTileBuffer == nullptr)
						return;

					if (!volume.infiniteBounce)
						return;

					auto group = registry.getRegistry().view<
						maple::component::MeshRenderer,
						maple::component::Transform,
						sdf::component::MeshDistanceField,
						component::MeshSurfaceAtlas
					>();

					surface.indirectLightDescriptor->setStorageBuffer("SDFAtlasTileBuffer", surfacePublic.ssboTileBuffer);
					surface.indirectLightDescriptor->setTexture("uIrradiance", volume.currentIrrdance);
					surface.indirectLightDescriptor->setTexture("uDepth", volume.currentDepth);
					surface.indirectLightDescriptor->setTexture("uColorSampler", surfacePublic.surfaceGBuffer0);
					surface.indirectLightDescriptor->setTexture("uNormalSampler", surfacePublic.surfaceGBuffer1);
					surface.indirectLightDescriptor->setTexture("uPBRSampler", surfacePublic.surfaceGBuffer2);
					surface.indirectLightDescriptor->setUniformBufferData("DDGIUBO", &uniform);
					auto cameraPos = glm::vec4(cameraView.cameraTransform->getWorldPosition(), volume.intensity);
					surface.indirectLightDescriptor->setUniform("UniformBufferObject", "cameraPos", glm::value_ptr(cameraPos));
					surface.indirectLightDescriptor->update(renderData.commandBuffer);
					surface.vertexBufferArray.clear();

					for (auto entity : surface.updatedObjects)
					{
						auto [_1, _2, _3, atlas] = group.get(entity);

						for (auto tileIndex = 0; tileIndex < 6; tileIndex++)
						{
							auto tile = atlas.tiles[tileIndex];
							if (tile == nullptr)
								continue;
							on_render::addTiles2(surface, tile);
						}
					}

					if (!surface.vertexBufferArray.empty())
					{
						PipelineInfo info;
						info.polygonMode = PolygonMode::Fill;
						info.cullMode = CullMode::None;
						info.blendMode = BlendMode::Add;        //add mode ..
						info.swapChainTarget = false;
						info.clearTargets = false;
						info.depthFunc = StencilType::Always;
						info.transparencyEnabled = true;
						info.depthTest = false;
						info.colorTargets[0] = surfacePublic.surfaceLightCache;
						info.pipelineName = "SDFAtlasIndirectLight";

						surface.vertexBuffer->setData(surface.vertexBufferArray.size() * sizeof(AtlasTileVertex), surface.vertexBufferArray.data());
						info.shader = Shader::create("shaders/SDF/SDFAtlasIndirectLight.shader");
						auto pipeline = Pipeline::get(info);
						pipeline->bind(renderData.commandBuffer);
						Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { surface.indirectLightDescriptor });
						surface.vertexBuffer->bind(renderData.commandBuffer, pipeline.get());
						renderDevice.device->drawInstanced(renderData.commandBuffer, surface.vertexBufferArray.size(), 1);
						surface.vertexBuffer->unbind();
						pipeline->end(renderData.commandBuffer);
					}

					surface.allLightingDirty = false;
				}
			}
		}        // namespace indirect_light
	}            // namespace surface

	auto SurfaceAtlasTile::onInsert(surface::component::MeshSurfaceAtlas& atlas, int32_t tileIndex) -> void
	{
		atlas.tiles[tileIndex] = this;
	}

	auto registerGlobalSurfaceAtlas(SystemQueue& begin, SystemQueue& render, SystemBuilder::Ptr builder) -> void
	{
		builder->registerGlobalComponent<surface::global::component::GlobalSurface>();
		builder->registerGlobalComponent<surface::global::component::GlobalSurfacePublic>();
		builder->addDependency<maple::component::Light, surface::component::LightFrameData>();
		builder->registerWithinQueue<surface::basic_update::system>(begin);
		builder->registerWithinQueue<surface::on_render::system>(render);
		builder->registerWithinQueue<surface::indirect_light::system>(render);
		builder->registerOnImGui<surface::on_imgui::system>();
	}
}        // namespace maple::sdf