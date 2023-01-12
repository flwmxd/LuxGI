//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "GLTFLoader.h"
#include "Engine/Material.h"
#include "Engine/Profiler.h"

#include "IO/MeshResource.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include "Scene/Component/Transform.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

#include "stb_image_resize.h"


namespace maple::io
{
	constexpr static char *AlbedoTexName   = "baseColorTexture";
	constexpr static char *NormalTexName   = "normalTexture";
	constexpr static char *MetallicTexName = "metallicRoughnessTexture";
	constexpr static char *GlossTexName    = "metallicRoughnessTexture";
	constexpr static char *AOTexName       = "occlusionTexture";
	constexpr static char *EmissiveTexName = "emissiveTexture";

	struct GLTFTexture
	{
		tinygltf::Image *  image;
		tinygltf::Sampler *sampler;
	};

	static std::unordered_map<int32_t, size_t> ComponentSize{
	    {TINYGLTF_COMPONENT_TYPE_BYTE, sizeof(int8_t)},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, sizeof(uint8_t)},
	    {TINYGLTF_COMPONENT_TYPE_SHORT, sizeof(int16_t)},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, sizeof(uint16_t)},
	    {TINYGLTF_COMPONENT_TYPE_INT, sizeof(int32_t)},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, sizeof(uint32_t)},
	    {TINYGLTF_COMPONENT_TYPE_FLOAT, sizeof(float)},
	    {TINYGLTF_COMPONENT_TYPE_DOUBLE, sizeof(double)}};

	static std::unordered_map<int32_t, int32_t> GLTF_COMPONENT_LENGTH_LOOKUP = {
	    {TINYGLTF_TYPE_SCALAR, 1},
	    {TINYGLTF_TYPE_VEC2, 2},
	    {TINYGLTF_TYPE_VEC3, 3},
	    {TINYGLTF_TYPE_VEC4, 4},
	    {TINYGLTF_TYPE_MAT2, 4},
	    {TINYGLTF_TYPE_MAT3, 9},
	    {TINYGLTF_TYPE_MAT4, 16}};

	static std::unordered_map<int32_t, int32_t> GLTF_COMPONENT_BYTE_SIZE_LOOKUP = {
	    {TINYGLTF_COMPONENT_TYPE_BYTE, 1},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1},
	    {TINYGLTF_COMPONENT_TYPE_SHORT, 2},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2},
	    {TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, 4},
	    {TINYGLTF_COMPONENT_TYPE_FLOAT, 4}};

	namespace
	{
		inline TextureWrap getWrapMode(int32_t mode)
		{
			switch (mode)
			{
				case TINYGLTF_TEXTURE_WRAP_REPEAT:
					return TextureWrap::Repeat;
				case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
					return TextureWrap::ClampToEdge;
				case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
					return TextureWrap::MirroredRepeat;
				default:
					return TextureWrap::Repeat;
			}
		}

		inline TextureFilter getFilter(int value)
		{
			switch (value)
			{
				case TINYGLTF_TEXTURE_FILTER_NEAREST:
				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
					return TextureFilter::Nearest;
				case TINYGLTF_TEXTURE_FILTER_LINEAR:
				case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
				case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
					return TextureFilter::Linear;
				default:
					return TextureFilter::Linear;
			}
		}

		inline auto loadMaterials(tinygltf::Model &gltfModel) -> std::vector<std::shared_ptr<Material>>
		{
			PROFILE_FUNCTION();
			std::vector<std::shared_ptr<Texture2D>> loadedTextures;
			std::vector<std::shared_ptr<Material>>  loadedMaterials;
			loadedTextures.resize(gltfModel.textures.size());
			loadedMaterials.reserve(gltfModel.materials.size());

			for (tinygltf::Texture &gltfTexture : gltfModel.textures)
			{
				GLTFTexture imageAndSampler{};

				if (gltfTexture.source != -1)
				{
					imageAndSampler.image = &gltfModel.images.at(gltfTexture.source);
				}

				if (gltfTexture.sampler != -1)
				{
					imageAndSampler.sampler = &gltfModel.samplers.at(gltfTexture.sampler);
				}

				if (imageAndSampler.image && loadedTextures[gltfTexture.source] == nullptr)
				{
					TextureParameters params;
					if (gltfTexture.sampler != -1)
						params = TextureParameters(getFilter(imageAndSampler.sampler->minFilter), getFilter(imageAndSampler.sampler->magFilter), getWrapMode(imageAndSampler.sampler->wrapS), getWrapMode(imageAndSampler.sampler->wrapT));

		
					if (imageAndSampler.image->width > 512 || imageAndSampler.image->height > 512)
					{
						auto scaleX = imageAndSampler.image->width / 512;
						auto scaleY = imageAndSampler.image->height / 512;
						auto resize = std::max(scaleY, scaleX);

						auto newW = imageAndSampler.image->width / resize;
						auto newH = imageAndSampler.image->height / resize;

						auto outputPixels = new uint8_t[newW * newH * 4 * sizeof(uint8_t)];
						stbir_resize_uint8(imageAndSampler.image->image.data(), imageAndSampler.image->width, imageAndSampler.image->height, 0, outputPixels, newW, newH, 0, 4);
						auto texture2D                     = Texture2D::create(newW, newH, outputPixels, params, {false, false, true});
						loadedTextures[gltfTexture.source] = texture2D;
						delete[] outputPixels;
					}
					else
					{
						auto texture2D                     = Texture2D::create(imageAndSampler.image->width, imageAndSampler.image->height, imageAndSampler.image->image.data(), params, {false, false, true});
						loadedTextures[gltfTexture.source] = texture2D;
					}
				}
			}

			auto textureName = [&](int32_t index) -> std::shared_ptr<Texture2D> {
				if (index >= 0)
				{
					const tinygltf::Texture &tex = gltfModel.textures[index];
					if (tex.source >= 0 && tex.source < loadedTextures.size())
					{
						return loadedTextures[tex.source];
					}
				}
				return nullptr;
			};

			for (tinygltf::Material &mat : gltfModel.materials)
			{
				PBRMataterialTextures textures;
				MaterialProperties    properties;
				auto                  pbrMaterial = std::make_shared<Material>();

				const auto &pbr = mat.pbrMetallicRoughness;

				textures.albedo   = textureName(pbr.baseColorTexture.index);
				textures.normal   = textureName(mat.normalTexture.index);
				textures.ao       = textureName(mat.occlusionTexture.index);
				textures.emissive = textureName(mat.emissiveTexture.index);
				textures.metallic = textureName(pbr.metallicRoughnessTexture.index);

				//TODO: correct way of handling this
				if (textures.metallic)
				{
					properties.workflow = PBR_WORKFLOW_METALLIC_ROUGHNESS;
				}
				else
				{
					properties.workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;
				}

				// metallic-roughness workflow:
				auto baseColourFactor = mat.values.find("baseColorFactor");
				auto roughnessFactor  = mat.values.find("roughnessFactor");
				auto metallicFactor   = mat.values.find("metallicFactor");

				if (roughnessFactor != mat.values.end())
				{
					properties.roughnessColor = glm::vec4(static_cast<float>(roughnessFactor->second.Factor()));
				}

				if (metallicFactor != mat.values.end())
				{
					properties.metallicColor = glm::vec4(static_cast<float>(metallicFactor->second.Factor()));
				}

				if (baseColourFactor != mat.values.end())
				{
					properties.albedoColor = glm::vec4((float) baseColourFactor->second.ColorFactor()[0], (float) baseColourFactor->second.ColorFactor()[1], (float) baseColourFactor->second.ColorFactor()[2], 1.0f);
				}

				// Extensions
				auto metallicGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
				if (metallicGlossinessWorkflow != mat.extensions.end())
				{
					if (metallicGlossinessWorkflow->second.Has("diffuseTexture"))
					{
						int index       = metallicGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
						textures.albedo = loadedTextures[gltfModel.textures[index].source];
					}

					if (metallicGlossinessWorkflow->second.Has("metallicGlossinessTexture"))
					{
						int index           = metallicGlossinessWorkflow->second.Get("metallicGlossinessTexture").Get("index").Get<int32_t>();
						textures.roughness  = loadedTextures[gltfModel.textures[index].source];
						properties.workflow = PBR_WORKFLOW_SPECULAR_GLOSINESS;
					}

					if (metallicGlossinessWorkflow->second.Has("diffuseFactor"))
					{
						auto &factor             = metallicGlossinessWorkflow->second.Get("diffuseFactor");
						properties.albedoColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int32_t>()) : 1.0f;
						properties.albedoColor.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int32_t>()) : 1.0f;
						properties.albedoColor.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int32_t>()) : 1.0f;
						properties.albedoColor.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int32_t>()) : 1.0f;
					}
					if (metallicGlossinessWorkflow->second.Has("metallicFactor"))
					{
						auto &factor               = metallicGlossinessWorkflow->second.Get("metallicFactor");
						properties.metallicColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int32_t>()) : 1.0f;
						properties.metallicColor.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int32_t>()) : 1.0f;
						properties.metallicColor.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int32_t>()) : 1.0f;
						properties.metallicColor.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int32_t>()) : 1.0f;
					}
					if (metallicGlossinessWorkflow->second.Has("glossinessFactor"))
					{
						auto &factor              = metallicGlossinessWorkflow->second.Get("glossinessFactor");
						properties.roughnessColor = glm::vec4(1.0f - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int32_t>()));
					}
				}

				properties.usingAlbedoMap = textures.albedo != nullptr ? 1.f : 0;
				properties.usingNormalMap = textures.normal != nullptr ? 1.f : 0;
				properties.usingAOMap     = textures.ao != nullptr ? 1.f : 0;

				if (properties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS && textures.metallic != nullptr)
				{
					properties.usingMetallicMap  = 1.0;
					properties.usingRoughnessMap = 0.0;
				}
				else
				{
					properties.usingMetallicMap  = textures.metallic != nullptr ? 1.f : 0;
					properties.usingRoughnessMap = textures.roughness != nullptr ? 1.f : 0;
				}
				properties.usingEmissiveMap = 0.f;

				pbrMaterial->setTextures(textures);
				pbrMaterial->setMaterialProperites(properties);

				if (mat.doubleSided)
					pbrMaterial->setRenderFlag(Material::RenderFlags::TwoSided);

				loadedMaterials.emplace_back(pbrMaterial);
			}
			return loadedMaterials;
		}

		inline auto loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh, std::vector<std::shared_ptr<Material>> &materials, component::Transform &parentTransform) -> std::vector<std::shared_ptr<Mesh>>
		{
			std::vector<std::shared_ptr<Mesh>> meshes;

			for (auto &primitive : mesh.primitives)
			{
				const auto &indicesAccessor = model.accessors[primitive.indices];

				std::vector<uint32_t> indices;
				std::vector<Vertex>   vertices;

				indices.resize(indicesAccessor.count);
				vertices.resize(indicesAccessor.count);

				for (auto &attribute : primitive.attributes)
				{
					// Get accessor info
					auto &accessor              = model.accessors.at(attribute.second);
					auto &bufferView            = model.bufferViews.at(accessor.bufferView);
					auto &buffer                = model.buffers.at(bufferView.buffer);
					int   componentLength       = GLTF_COMPONENT_LENGTH_LOOKUP.at(accessor.type);
					int   componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(accessor.componentType);

					// Extra vertex data from buffer
					size_t               bufferOffset = bufferView.byteOffset + accessor.byteOffset;
					int                  bufferLength = static_cast<int>(accessor.count) * componentLength * componentTypeByteSize;
					auto                 first        = buffer.data.begin() + bufferOffset;
					auto                 last         = buffer.data.begin() + bufferOffset + bufferLength;
					std::vector<uint8_t> data         = std::vector<uint8_t>(first, last);

					if (attribute.first == "POSITION")
					{
						size_t positionCount = accessor.count;
						auto   positions     = reinterpret_cast<glm::vec3 *>(data.data());
						for (auto p = 0; p < positionCount; ++p)
						{
							vertices[p].pos   = parentTransform.getWorldMatrix() * glm::vec4(positions[p], 1.0);
							vertices[p].color = {1, 1, 1, 1};
						}
					}

					else if (attribute.first == "NORMAL")
					{
						size_t normalCount = accessor.count;
						auto   normals     = reinterpret_cast<glm::vec3 *>(data.data());
						for (auto p = 0; p < normalCount; ++p)
						{
							vertices[p].normal = (parentTransform.getWorldMatrix() * glm::vec4(normals[p], 1.0));

							glm::normalize(vertices[p].normal);
						}
					}

					else if (attribute.first == "TEXCOORD_0")
					{
						size_t uvCount = accessor.count;
						auto   uvs     = reinterpret_cast<glm::vec2 *>(data.data());
						for (auto p = 0; p < uvCount; ++p)
						{
							vertices[p].texCoord = uvs[p];
						}
					}

					else if (attribute.first == "COLOR_0")
					{
						size_t uvCount = accessor.count;
						auto   colors  = reinterpret_cast<glm::vec4 *>(data.data());
						for (auto p = 0; p < uvCount; ++p)
						{
							vertices[p].color = colors[p];
						}
					}

					else if (attribute.first == "TANGENT")
					{
						size_t uvCount = accessor.count;
						auto   uvs     = reinterpret_cast<glm::vec3 *>(data.data());
						for (auto p = 0; p < uvCount; ++p)
						{
							vertices[p].tangent = parentTransform.getWorldMatrix() * glm::vec4(uvs[p], 1.0);
						}
					}
				}

				{
					// Get accessor info
					auto indexAccessor   = model.accessors.at(primitive.indices);
					auto indexBufferView = model.bufferViews.at(indexAccessor.bufferView);
					auto indexBuffer     = model.buffers.at(indexBufferView.buffer);

					int componentLength       = GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
					int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);

					// Extra index data
					size_t               bufferOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
					int                  bufferLength = static_cast<int>(indexAccessor.count) * componentLength * componentTypeByteSize;
					auto                 first        = indexBuffer.data.begin() + bufferOffset;
					auto                 last         = indexBuffer.data.begin() + bufferOffset + bufferLength;
					std::vector<uint8_t> data         = std::vector<uint8_t>(first, last);

					size_t indicesCount = indexAccessor.count;
					if (componentTypeByteSize == 1)
					{
						uint8_t *in = reinterpret_cast<uint8_t *>(data.data());
						for (auto iCount = 0; iCount < indicesCount; iCount++)
						{
							indices[iCount] = (uint32_t) in[iCount];
						}
					}
					else if (componentTypeByteSize == 2)
					{
						uint16_t *in = reinterpret_cast<uint16_t *>(data.data());
						for (auto iCount = 0; iCount < indicesCount; iCount++)
						{
							indices[iCount] = (uint32_t) in[iCount];
						}
					}
					else if (componentTypeByteSize == 4)
					{
						auto in = reinterpret_cast<uint32_t *>(data.data());
						for (auto iCount = 0; iCount < indicesCount; iCount++)
						{
							indices[iCount] = in[iCount];
						}
					}
					else
					{
						LOGW("Unsupported indices data type - {0}", componentTypeByteSize);
					}
				}
				meshes.emplace_back(std::make_shared<Mesh>(indices, vertices));
			}
			return meshes;
		}

		inline auto loadNode(const std::string &parentName, int32_t nodeIndex, const glm::mat4 &parentTransform,
		                     tinygltf::Model &model, std::vector<std::shared_ptr<Material>> &materials, std::unordered_map<std::string, std::shared_ptr<Mesh>> &out)
		{
			PROFILE_FUNCTION();
			if (nodeIndex < 0)
			{
				return;
			}

			auto &node = model.nodes[nodeIndex];
			auto  name = node.name;

			component::Transform transform;
			glm::mat4            matrix;
			glm::mat4            position = glm::mat4(1.0f);
			glm::mat4            rotation = glm::mat4(1.0f);
			glm::mat4            scale    = glm::mat4(1.0f);

			if (!node.scale.empty())
			{
				scale = glm::scale(glm::mat4(1.0),
				                   glm::vec3(
				                       static_cast<float>(node.scale[0]),
				                       static_cast<float>(node.scale[1]),
				                       static_cast<float>(node.scale[2])));
			}

			if (!node.rotation.empty())
			{
				rotation = glm::toMat4(glm::quat(
				    static_cast<float>(node.rotation[3]),
				    static_cast<float>(node.rotation[0]),
				    static_cast<float>(node.rotation[1]),
				    static_cast<float>(node.rotation[2])));
			}

			if (!node.translation.empty())
			{
				position = glm::translate(glm::mat4(1.0),
				                          glm::vec3(
				                              static_cast<float>(node.translation[0]),
				                              static_cast<float>(node.translation[1]),
				                              static_cast<float>(node.translation[2])));
			}

			if (!node.matrix.empty())
			{
				float matrixData[16];
				for (int i = 0; i < 16; i++)
					matrixData[i] = float(node.matrix.data()[i]);
				matrix = glm::make_mat4(matrixData);
				transform.setLocalTransform(matrix);
			}
			else
			{
				matrix = position * rotation * scale;
				transform.setLocalTransform(matrix);
			}

			transform.applyTransform();
			transform.setWorldMatrix(parentTransform);

			if (node.mesh >= 0)
			{
				int32_t subIndex = 0;

				auto meshes = loadMesh(model, model.meshes[node.mesh], materials, transform);

				for (auto &mesh : meshes)
				{
					std::string subname = node.name;

					if (subname == "")
					{
						subname = parentName + "_" + std::to_string(subIndex);
					}

					mesh->setName(subname);

					int32_t materialIndex = model.meshes[node.mesh].primitives[subIndex].material;
					if (materialIndex >= 0)
						mesh->setMaterial(materials[materialIndex]);

					out[subname] = mesh;
					subIndex++;
				}
			}

			if (!node.children.empty())
			{
				for (int32_t child : node.children)
				{
					auto name = parentName + "_child_" + std::to_string(child);
					loadNode(name, child, transform.getLocalMatrix(), model, materials, out);
				}
			}
		}
	}        // namespace

	auto GLTFLoader::load(const std::string &obj, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void
	{
		PROFILE_FUNCTION();
		auto               name = StringUtils::getFileNameWithoutExtension(obj);
		tinygltf::Model    model;
		tinygltf::TinyGLTF loader;
		std::string        err;
		std::string        warn;

		bool ret;

		stbi_set_flip_vertically_on_load(0);

		if (extension == "glb")        // assume binary glTF.
		{
			PROFILE_SCOPE(".glb binary loading");
			ret = tinygltf::TinyGLTF().LoadBinaryFromFile(&model, &err, &warn, obj);
		}
		else        // assume ascii glTF.
		{
			PROFILE_SCOPE(".gltf loading");
			ret = tinygltf::TinyGLTF().LoadASCIIFromFile(&model, &err, &warn, obj);
		}

		if (!err.empty())
		{
			LOGE(err);
		}

		if (!warn.empty())
		{
			LOGW(warn);
		}

		auto meshRes = std::make_shared<MeshResource>(obj);

		out.emplace_back(meshRes);

		if (ret)
		{
			PROFILE_SCOPE("Parse GLTF Model");

			auto loadedMaterials = loadMaterials(model);

			const tinygltf::Scene &gltfScene = model.scenes[std::max(0, model.defaultScene)];
			for (size_t i = 0; i < gltfScene.nodes.size(); i++)
			{
				loadNode(name, gltfScene.nodes[i], glm::mat4(1.0f), model, loadedMaterials, meshRes->getMeshes());
			}
		}
		stbi_set_flip_vertically_on_load(1);
	}
};        // namespace maple::io
