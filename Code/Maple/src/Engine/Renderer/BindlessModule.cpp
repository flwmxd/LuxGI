//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "BindlessModule.h"

#include "Engine/Mesh.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"
#include "Engine/Raytrace/RaytraceConfig.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "IoC/SystemBuilder.h"

#include "Engine/Raytrace/TracedData.h"

#include "RHI/AccelerationStructure.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include <scope_guard.hpp>

namespace maple
{
	namespace
	{
	}        // namespace

	namespace bindless
	{
		namespace clear_queue
		{
			inline auto system(global::component::MaterialChanged& changed)
			{
				if (!changed.updateQueue.empty())
				{
					changed.updateQueue.clear();
				}
			}
		}        // namespace clear_queue

		namespace gather_scene
		{
			inline auto updateMaterial(raytracing::MaterialData& materialData,
				Material* material,
				global::component::Bindless& bindless,
				std::vector<Texture::Ptr>& shaderTextures,
				std::unordered_map<uint32_t, uint32_t>& textureIndices,
				const skybox_renderer::global::component::SkyboxData* skybox)
			{
				materialData.textureIndices0 = glm::ivec4(-1);
				materialData.textureIndices1 = glm::ivec4(-1);

				materialData.albedo = material->getProperties().albedoColor;
				materialData.roughness = material->getProperties().roughnessColor;
				materialData.metalic = material->getProperties().metallicColor;
				materialData.emissive = material->getProperties().emissiveColor;

				materialData.usingValue0 = {
					material->getProperties().usingAlbedoMap,
					material->getProperties().usingMetallicMap,
					material->getProperties().usingRoughnessMap,
					0 };

				materialData.usingValue1 = {
					material->getProperties().usingNormalMap,
					material->getProperties().usingAOMap,
					material->getProperties().usingEmissiveMap,
					material->getProperties().workflow };

				auto textures = material->getMaterialTextures();

				if (textures.albedo)
				{
					if (textureIndices.count(textures.albedo->getId()) == 0)
					{
						materialData.textureIndices0.x = shaderTextures.size();
						shaderTextures.emplace_back(textures.albedo);
						textureIndices.emplace(textures.albedo->getId(), materialData.textureIndices0.x);
					}
					else
					{
						materialData.textureIndices0.x = textureIndices[textures.albedo->getId()];
					}
				}

				if (textures.normal)
				{
					if (textureIndices.count(textures.normal->getId()) == 0)
					{
						materialData.textureIndices0.y = shaderTextures.size();
						shaderTextures.emplace_back(textures.normal);
						textureIndices.emplace(textures.normal->getId(), materialData.textureIndices0.y);
					}
					else
					{
						materialData.textureIndices0.y = textureIndices[textures.normal->getId()];
					}
				}

				if (textures.roughness)
				{
					if (textureIndices.count(textures.roughness->getId()) == 0)
					{
						materialData.textureIndices0.z = shaderTextures.size();
						shaderTextures.emplace_back(textures.roughness);
						textureIndices.emplace(textures.roughness->getId(), materialData.textureIndices0.z);
					}
					else
					{
						materialData.textureIndices0.z = textureIndices[textures.roughness->getId()];
					}
				}

				if (textures.metallic)
				{
					if (textureIndices.count(textures.metallic->getId()) == 0)
					{
						materialData.textureIndices0.w = shaderTextures.size();
						shaderTextures.emplace_back(textures.metallic);
						textureIndices.emplace(textures.metallic->getId(), materialData.textureIndices0.w);
					}
					else
					{
						materialData.textureIndices0.w = textureIndices[textures.metallic->getId()];
					}
				}

				if (textures.emissive)
				{
					if (textureIndices.count(textures.emissive->getId()) == 0)
					{
						materialData.textureIndices1.x = shaderTextures.size();
						shaderTextures.emplace_back(textures.emissive);
						textureIndices.emplace(textures.emissive->getId(), materialData.textureIndices1.x);
					}
					else
					{
						materialData.textureIndices1.x = textureIndices[textures.emissive->getId()];
					}
				}

				if (textures.ao)
				{
					if (textureIndices.count(textures.ao->getId()) == 0)
					{
						materialData.textureIndices1.y = shaderTextures.size();
						shaderTextures.emplace_back(textures.ao);
						textureIndices.emplace(textures.ao->getId(), materialData.textureIndices1.y);
					}
					else
					{
						materialData.textureIndices1.y = textureIndices[textures.ao->getId()];
					}
				}
			}

			inline auto system(ioc::Registry registry,
				const skybox_renderer::global::component::SkyboxData* skybox,
				const maple::component::RendererData& rendererData,
				const maple::raytracing::global::component::TopLevelAs& topLevels,
				global::component::Bindless& bindless,
				global::component::GraphicsContext& context,
				global::component::RaytracingDescriptor& descriptor,
				global::component::SceneTransformChanged* sceneChanged,
				const trace::global::component::RaytraceConfig& config,
				global::component::MaterialChanged* materialChanged)
			{
				if (!context.context->isRaytracingSupported() || trace::isSoftTrace(config))
					return;

				if (bindless.meshIndices.empty())
					return;

				auto lightGroup = registry.getRegistry().view<component::Light, component::Transform>();
				auto meshGroup = registry.getRegistry().view<component::MeshRenderer, component::Transform>();

				if (sceneChanged && sceneChanged->dirty && topLevels.topLevelAs)
				{
					descriptor.updated = true;
					std::unordered_set<uint32_t>           processedMeshes;
					std::unordered_set<uint32_t>           processedMaterials;
					std::unordered_map<uint32_t, uint32_t> globalMaterialIndices;
					uint32_t                               meshCounter = 0;
					uint32_t                               gpuMaterialCounter = 0;

					std::vector<VertexBuffer::Ptr> vbos;
					std::vector<IndexBuffer::Ptr>  ibos;
					std::vector<StorageBuffer::Ptr> materialIndices;
					materialIndices.resize(std::distance(meshGroup.begin(), meshGroup.end()));

					context.context->waitIdle();

					auto meterialBuffer = (raytracing::MaterialData*)descriptor.materialBuffer->map();
					auto transformBuffer = (raytracing::TransformData*)descriptor.transformBuffer->map();
					auto lightBuffer = (component::LightData*)descriptor.lightBuffer->map();

					auto tasks = BatchTask::create();
					std::unordered_map<uint32_t, uint32_t> vertexMapping;

					for (auto [entity, mesh, transform] : meshGroup.each())
					{
						if (processedMeshes.count(mesh.mesh->getId()) == 0)
						{
							processedMeshes.emplace(mesh.mesh->getId());
							vertexMapping[mesh.mesh->getId()] = vbos.size();
							vbos.emplace_back(mesh.mesh->getVertexBuffer());
							ibos.emplace_back(mesh.mesh->getIndexBuffer());

							MAPLE_ASSERT(mesh.mesh->getSubMeshCount() != 0, "sub mesh should be one at least");

							for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
							{
								auto material = mesh.mesh->getMaterial(i);

								if (material == nullptr)
								{
									material = descriptor.defaultMaterial.get();
								}

								if (processedMaterials.count(material->getId()) == 0)
								{
									processedMaterials.emplace(material->getId());

									auto& materialData = meterialBuffer[gpuMaterialCounter];
									updateMaterial(materialData, material, bindless, descriptor.shaderTextures, bindless.textureIndices, skybox);
									bindless.materialIndices[material->getId()] = gpuMaterialCounter;
									gpuMaterialCounter++;
								}
							}
						}

						auto buffer = mesh.mesh->getSubMeshesBuffer();

						auto indices = (glm::uvec2*)buffer->map();


						for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
						{
							auto material = mesh.mesh->getMaterial(i);
							if (material == nullptr)
							{
								material = descriptor.defaultMaterial.get();
							}
							const auto& subIndex = mesh.mesh->getSubMeshIndex();
							indices[i] = glm::uvec2(i == 0 ? 0 : subIndex[i - 1] / 3, bindless.materialIndices[material->getId()]);
						}

						auto blas = mesh.mesh->getAccelerationStructure(tasks);

						auto instanceId = bindless.meshIndices[(uint32_t)entity];
						materialIndices[instanceId] = buffer;
						//vertex buffer Id;
						transformBuffer[instanceId].meshIndex = vertexMapping[mesh.mesh->getId()];
						transformBuffer[instanceId].model = transform.getWorldMatrix();
						transformBuffer[instanceId].normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform.getWorldMatrix())));
						buffer->unmap();
					}

					uint32_t lightIndicator = 0;

					for (auto [lightEntity, light, transform] : lightGroup.each())
					{
						light.lightData.position = { transform.getWorldPosition(), 1.f };
						light.lightData.direction = { glm::normalize(transform.getWorldOrientation() * maple::FORWARD), light.lightData.direction.w };
						lightBuffer[lightIndicator++] = light.lightData;
					}

					descriptor.sceneDescriptor->setTexture("uSkybox", rendererData.unitCube);

					if (skybox && skybox->skybox != nullptr)
					{
						descriptor.sceneDescriptor->setTexture("uSkybox", skybox->skybox);
					}

					descriptor.numLights = lightIndicator;

					tasks->execute();

					descriptor.materialBuffer->unmap();
					descriptor.transformBuffer->unmap();
					descriptor.lightBuffer->unmap();

					descriptor.sceneDescriptor->setStorageBuffer("MaterialBuffer", descriptor.materialBuffer);
					descriptor.sceneDescriptor->setStorageBuffer("TransformBuffer", descriptor.transformBuffer);
					descriptor.sceneDescriptor->setStorageBuffer("LightBuffer", descriptor.lightBuffer);
					descriptor.sceneDescriptor->setAccelerationStructure("uTopLevelAS", topLevels.topLevelAs);

					descriptor.vboDescriptor->setStorageBuffer("VertexBuffer", vbos);
					descriptor.iboDescriptor->setStorageBuffer("IndexBuffer", ibos);

					descriptor.materialDescriptor->setStorageBuffer("SubmeshInfoBuffer", materialIndices);
					descriptor.textureDescriptor->setTexture("uSamplers", descriptor.shaderTextures);
				}
			}
		}        // namespace gather_scene

		namespace update_command
		{
			inline auto system(global::component::RaytracingDescriptor& descriptor, const maple::component::RendererData& rendererData)
			{
				if (descriptor.updated)
				{
					descriptor.updated = false;
					descriptor.sceneDescriptor->update(rendererData.commandBuffer);
					descriptor.vboDescriptor->update(rendererData.commandBuffer);
					descriptor.iboDescriptor->update(rendererData.commandBuffer);
					descriptor.materialDescriptor->update(rendererData.commandBuffer);
					descriptor.textureDescriptor->update(rendererData.commandBuffer);
				}
			}
		}        // namespace update_command

		auto registerBindlessModule(SystemQueue& begin, SystemQueue& renderer, std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerWithinQueue<gather_scene::system>(begin);
			builder->registerWithinQueue<update_command::system>(renderer);

			builder->registerGlobalComponent<global::component::RaytracingDescriptor>([=](auto& descriptor) {
				if (builder->getGlobalComponent<global::component::GraphicsContext>().context->isRaytracingSupported())
				{

					auto shader = Shader::create("shaders/DDGI/GIRays.shader", { {"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
																						{"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT} });

					descriptor.lightBuffer = StorageBuffer::create(sizeof(component::LightData) * MAX_SCENE_LIGHT_COUNT, nullptr, BufferOptions{ false,  MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });
					descriptor.materialBuffer = StorageBuffer::create(sizeof(raytracing::MaterialData) * MAX_SCENE_MATERIAL_COUNT, nullptr, BufferOptions{ false, MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });
					descriptor.transformBuffer = StorageBuffer::create(sizeof(raytracing::TransformData) * MAX_SCENE_MESH_INSTANCE_COUNT, nullptr, BufferOptions{ false,  MemoryUsage::MEMORY_USAGE_CPU_TO_GPU });

					descriptor.descriptorPool = DescriptorPool::create({ 25,
																		{{DescriptorType::UniformBufferDynamic, 10},
																		 {DescriptorType::ImageSampler, MAX_SCENE_MATERIAL_TEXTURE_COUNT},
																		 {DescriptorType::Buffer, 5 * MAX_SCENE_MESH_INSTANCE_COUNT},
																		 {DescriptorType::AccelerationStructure, 10}} });

					descriptor.sceneDescriptor = DescriptorSet::create({ 0, shader.get(), 1, descriptor.descriptorPool.get() });
					descriptor.vboDescriptor = DescriptorSet::create({ 1, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT });
					descriptor.iboDescriptor = DescriptorSet::create({ 2, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT });
					descriptor.materialDescriptor = DescriptorSet::create({ 3, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT });
					descriptor.textureDescriptor = DescriptorSet::create({ 4, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MATERIAL_TEXTURE_COUNT });

					descriptor.sceneDescriptor->setName("SceneDescriptor");
					descriptor.vboDescriptor->setName("VBO-Descriptor");
					descriptor.iboDescriptor->setName("IBO-Descriptor");
					descriptor.materialDescriptor->setName("MaterialDescriptor");
					descriptor.textureDescriptor->setName("TextureDescriptor");

					descriptor.defaultMaterial = std::make_shared<Material>();
					MaterialProperties properties;
					properties.albedoColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
					properties.roughnessColor = glm::vec4(1.f);
					properties.metallicColor = glm::vec4(0);
					properties.usingAlbedoMap = 0.0f;
					properties.usingRoughnessMap = 0.0f;
					properties.usingNormalMap = 0.0f;
					properties.usingMetallicMap = 0.0f;
					descriptor.defaultMaterial->setMaterialProperites(properties);
				}
				});
		}

		auto registerBindless(std::shared_ptr<SystemBuilder> builder) -> void
		{
			builder->registerSystemInFrameEnd<clear_queue::system>();
		}
	}        // namespace bindless
}        // namespace maple