//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "OBJLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "IO/File.h"
#include "IO/MeshResource.h"
#include "Engine/Material.h"
#include "Engine/Profiler.h"
#include "Others/StringUtils.h"
#include "RHI/Texture.h"
#include <tiny_obj_loader.h>

namespace maple::io
{
	std::shared_ptr<Texture2D> loadMaterialTextures(const std::string &typeName, std::vector<std::shared_ptr<Texture2D>> &texturesLoaded, const std::string &name, const std::string &directory, TextureParameters format)
	{
		for (uint32_t j = 0; j < texturesLoaded.size(); j++)
		{
			if (std::strcmp(texturesLoaded[j]->getFilePath().c_str(), (directory + "/" + name).c_str()) == 0)
			{
				return texturesLoaded[j];
			}
		}

		{
			if (File::fileExists(directory + "/" + name))
			{
				TextureLoadOptions options{false, false, true};
				auto               texture = Texture2D::create(typeName, directory + "/" + name, format, options);
				texturesLoaded.emplace_back(texture);
				return texture;
			}
		}
		return nullptr;
	}

	auto OBJLoader::load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void
	{
		PROFILE_FUNCTION();
		std::vector<std::shared_ptr<Texture2D>> texturesCache;

		std::string resolvedPath = fileName;
		auto        directory    = resolvedPath.substr(0, resolvedPath.find_last_of(StringUtils::delimiter));
		std::string name         = directory.substr(directory.find_last_of(StringUtils::delimiter) + 1);

		tinyobj::attrib_t                attrib;
		std::vector<tinyobj::shape_t>    shapes;
		std::vector<tinyobj::material_t> materials;
		std::string                      warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str(), directory.c_str()))
		{
			throw std::runtime_error(warn + err);
		}

		auto meshes = std::make_shared<MeshResource>(fileName);

		for (const auto &shape : shapes)
		{
			std::vector<Vertex>                  vertices;
			std::vector<uint32_t>                indices;
			std::unordered_map<Vertex, uint32_t> uniqueVertices{};

			for (const auto &index : shape.mesh.indices)
			{
				Vertex vertex{};

				vertex.pos = {
				    attrib.vertices[3 * index.vertex_index + 0],
				    attrib.vertices[3 * index.vertex_index + 1],
				    attrib.vertices[3 * index.vertex_index + 2]};

				if (index.normal_index >= 0)
					vertex.normal = {
					    attrib.normals[3 * index.normal_index + 0],
					    attrib.normals[3 * index.normal_index + 1],
					    attrib.normals[3 * index.normal_index + 2]};

				if (index.texcoord_index >= 0)
					vertex.texCoord = {
					    attrib.texcoords[2 * index.texcoord_index + 0],
					    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

				vertex.color = {1.0f, 1.0f, 1.0f, 1.f};

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);
			}

			if (indices.empty())
				continue;

			if (attrib.normals.empty())
				Mesh::generateNormals(vertices, indices);

			Mesh::generateTangents(vertices, indices);
			//Mesh::generateBitangents(vertices, indices);

			std::vector<std::shared_ptr<Material>> pbrMaterials;
			std::vector<uint32_t>                  subMeshIndices;
			auto                                   mesh = std::make_shared<Mesh>(indices, vertices);

			std::unordered_map<uint32_t, std::shared_ptr<Material>> materialCache;

			if (!shape.mesh.material_ids.empty())
			{
				uint32_t prevIdx = -1;
				uint32_t index   = 0;

				for (auto id : shape.mesh.material_ids)
				{
					if (prevIdx != id)
					{
						if (index != 0)
							subMeshIndices.emplace_back(index * 3);

						prevIdx = id;

						tinyobj::material_t *mp = &materials[id];

						auto & pbrMaterial = materialCache[id];
						if (pbrMaterial == nullptr)
						{
							pbrMaterial.reset(new Material());

							PBRMataterialTextures textures;

							if (mp->diffuse_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Albedo",
								                                                          texturesCache,
								                                                          mp->diffuse_texname, directory, TextureParameters(TextureFilter::Linear, TextureFilter::Linear, mp->diffuse_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
									textures.albedo = texture;
							}
							else
							{
								pbrMaterial->getProperties().albedoColor = {mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.f};
							}

							if (mp->normal_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Normal",
								                                                          texturesCache, mp->normal_texname, directory, TextureParameters(TextureFilter::Linear, TextureFilter::Linear, mp->normal_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
									textures.normal = texture;
							}

							else if (mp->bump_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Normal",
								                                                          texturesCache, mp->bump_texname, directory, TextureParameters(TextureFilter::Linear, TextureFilter::Linear, mp->normal_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
									textures.normal = texture;
							}

							if (mp->roughness_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Roughness", texturesCache, mp->roughness_texname.c_str(), directory,
								                                                          TextureParameters(
								                                                              TextureFilter::Linear,
								                                                              TextureFilter::Linear, mp->roughness_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
									textures.roughness = texture;
							}
							else
							{
								//pbrMaterial->getProperties().roughnessColor = {mp->roughness, mp->roughness, mp->roughness, 1.f};
							}

							if (mp->metallic_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Metallic", texturesCache, mp->metallic_texname, directory,
								                                                          TextureParameters(
								                                                              TextureFilter::Linear,
								                                                              TextureFilter::Linear, mp->metallic_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
									textures.metallic = texture;
							}
							else
							{
								pbrMaterial->getProperties().metallicColor = {mp->metallic, mp->metallic, mp->metallic, 1.f};
							}

							if (mp->emissive_texname.length() > 0)
							{
								std::shared_ptr<Texture2D> texture = loadMaterialTextures("Emissive", texturesCache, mp->emissive_texname, directory,
								                                                          TextureParameters(
								                                                              TextureFilter::Linear,
								                                                              TextureFilter::Linear, mp->emissive_texopt.clamp ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
								if (texture)
								{
									textures.emissive = texture;
								}
							}
							else
							{
								pbrMaterial->getProperties().emissiveColor = {mp->emission[0], mp->emission[1], mp->emission[2], 1.f};
							}

							pbrMaterial->setTextures(textures);
						}

						pbrMaterials.emplace_back(pbrMaterial);
					}
					index++;
				}

				if (subMeshIndices.size() > 1)
				{
					subMeshIndices.emplace_back(index * 3);
					mesh->setSubMeshIndex(subMeshIndices);
					MAPLE_ASSERT(subMeshIndices.size() == pbrMaterials.size(), "size must be same");
				}
			}

			mesh->setMaterial(pbrMaterials);
			mesh->setName(shape.name);
			meshes->addMesh(shape.name, mesh);
		}
		out.emplace_back(meshes);
	}
};        // namespace maple::io
