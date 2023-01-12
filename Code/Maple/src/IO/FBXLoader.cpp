//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "FBXLoader.h"
#include "IO/File.h"
#include "IO/MeshResource.h"

#include "Engine/Core.h"
#include "Engine/Material.h"

#include "Math/BoundingBox.h"
#include "Math/MathUtils.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include "Scene/Component/Transform.h"

#include <mio/mmap.hpp>
#include <ofbx.h>
#include <vector>

namespace maple::io
{
	enum class Orientation
	{
		Y_UP,
		Z_UP,
		Z_MINUS_UP,
		X_MINUS_UP,
		X_UP
	};

	namespace
	{
		static auto operator-(const ofbx::Vec3 &a, const ofbx::Vec3 &b) -> ofbx::Vec3
		{
			return {a.x - b.x, a.y - b.y, a.z - b.z};
		}

		static auto operator-(const ofbx::Vec2 &a, const ofbx::Vec2 &b) -> ofbx::Vec2
		{
			return {a.x - b.x, a.y - b.y};
		}

		inline auto getBonePath(const ofbx::Object *bone) -> std::string
		{
			if (bone == nullptr)
			{
				return "";
			}
			return getBonePath(bone->getParent()) + "/" + bone->name;
		}

		inline auto toGlm(const ofbx::Vec2 &vec)
		{
			return glm::vec2(float(vec.x), float(vec.y));
		}

		inline auto toGlm(const ofbx::Vec3 &vec)
		{
			return glm::vec3(float(vec.x), float(vec.y), float(vec.z));
		}

		inline auto toGlm(const ofbx::Vec4 &vec)
		{
			return glm::vec4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
		}

		inline auto toGlm(const ofbx::Color &vec)
		{
			return glm::vec4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
		}

		inline auto fixOrientation(const glm::vec3 &v, Orientation orientation) -> glm::vec3
		{
			switch (orientation)
			{
				case Orientation::Y_UP:
					return glm::vec3(v.x, v.y, v.z);
				case Orientation::Z_UP:
					return glm::vec3(v.x, v.z, -v.y);
				case Orientation::Z_MINUS_UP:
					return glm::vec3(v.x, -v.z, v.y);
				case Orientation::X_MINUS_UP:
					return glm::vec3(v.y, -v.x, v.z);
				case Orientation::X_UP:
					return glm::vec3(-v.y, v.x, v.z);
			}
			return v;
		}

		inline auto fixOrientation(const glm::quat &v, Orientation orientation) -> glm::quat
		{
			switch (orientation)
			{
				case Orientation::Y_UP:
					return {v.w, v.x, v.y, v.z};
				case Orientation::Z_UP:
					return {v.w, v.x, -v.y, v.z};
				case Orientation::Z_MINUS_UP:
					return {v.w, v.x, -v.z, v.y};
				case Orientation::X_MINUS_UP:
					return {v.w, -v.x, v.z, v.y};
				case Orientation::X_UP:
					return {-v.y, v.x, v.z, v.w};
			}
			return v;
		}

		inline auto computeTangents(ofbx::Vec3 *out, int32_t vertexCount, const ofbx::Vec3 *vertices, const ofbx::Vec3 *normals, const ofbx::Vec2 *uvs)
		{
			for (int i = 0; i < vertexCount; i += 3)
			{
				const auto &v0  = vertices[i + 0];
				const auto &v1  = vertices[i + 1];
				const auto &v2  = vertices[i + 2];
				const auto &uv0 = uvs[i + 0];
				const auto &uv1 = uvs[i + 1];
				const auto &uv2 = uvs[i + 2];

				const ofbx::Vec3 dv10  = v1 - v0;
				const ofbx::Vec3 dv20  = v2 - v0;
				const ofbx::Vec2 duv10 = uv1 - uv0;
				const ofbx::Vec2 duv20 = uv2 - uv0;

				const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
				ofbx::Vec3  tangent;
				tangent.x     = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
				tangent.y     = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
				tangent.z     = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
				const float l = 1 / sqrtf(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
				tangent.x *= l;
				tangent.y *= l;
				tangent.z *= l;
				out[i + 0] = tangent;
				out[i + 1] = tangent;
				out[i + 2] = tangent;
			}
		}

		template <typename T>
		inline auto toMatrix(const ofbx::Matrix &mat)
		{
			T result;
			for (int32_t i = 0; i < 4; i++)
				for (int32_t j = 0; j < 4; j++)
					result[i][j] = (float) mat.m[i * 4 + j];
			return result;
		}

		inline auto getTransform(const ofbx::Object *object, Orientation orientation) -> component::Transform
		{
			component::Transform transform;
			ofbx::Vec3           p = object->getLocalTranslation();
			transform.setLocalPosition(fixOrientation({static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)}, orientation));
			ofbx::Vec3 r = object->getLocalRotation();
			transform.setLocalOrientation(fixOrientation(glm::vec3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)), orientation));
			ofbx::Vec3 s = object->getLocalScaling();
			transform.setLocalScale({static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z)});

			if (object->getParent())
			{
				transform.setWorldMatrix(getTransform(object->getParent(), orientation).getWorldMatrix());
			}
			else
			{
				transform.setWorldMatrix(glm::mat4(1));
			}
			return transform;
		}

		inline auto loadTexture(const ofbx::Material *material, ofbx::Texture::TextureType type) -> std::shared_ptr<Texture2D>
		{
			const ofbx::Texture *      ofbxTexture = material->getTexture(type);
			std::shared_ptr<Texture2D> texture2D;
			if (ofbxTexture)
			{
				ofbx::DataView filename = ofbxTexture->getRelativeFileName();
				if (filename == "")
					filename = ofbxTexture->getFileName();

				char filePath[256];
				filename.toString(filePath);

				if (File::fileExists(filePath))
				{
					texture2D = Texture2D::create(filePath, filePath);
				}
				else
				{
					LOGW("file {0} did not find", filePath);
				}
			}

			return texture2D;
		}

		inline auto loadMaterial(const ofbx::Material *material, bool animated)
		{
			auto pbrMaterial = std::make_shared<Material>();

			PBRMataterialTextures textures;
			MaterialProperties    properties;

			properties.albedoColor   = toGlm(material->getDiffuseColor());
			properties.metallicColor = toGlm(material->getSpecularColor());

			float roughness           = 1.0f - std::sqrt(float(material->getShininess()) / 100.0f);
			properties.roughnessColor = glm::vec4(roughness);

			textures.albedo    = loadTexture(material, ofbx::Texture::TextureType::DIFFUSE);
			textures.normal    = loadTexture(material, ofbx::Texture::TextureType::NORMAL);
			textures.metallic  = loadTexture(material, ofbx::Texture::TextureType::SPECULAR);
			textures.roughness = loadTexture(material, ofbx::Texture::TextureType::SHININESS);
			textures.emissive  = loadTexture(material, ofbx::Texture::TextureType::EMISSIVE);
			textures.ao        = loadTexture(material, ofbx::Texture::TextureType::AMBIENT);

			if (!textures.albedo)
				properties.usingAlbedoMap = 0.0f;
			if (!textures.normal)
				properties.usingNormalMap = 0.0f;
			if (!textures.metallic)
				properties.usingMetallicMap = 0.0f;
			if (!textures.roughness)
				properties.usingRoughnessMap = 0.0f;
			if (!textures.emissive)
				properties.usingEmissiveMap = 0.0f;
			if (!textures.ao)
				properties.usingAOMap = 0.0f;

			pbrMaterial->setTextures(textures);
			pbrMaterial->setMaterialProperites(properties);

			return pbrMaterial;
		}

		inline auto getOffsetMatrix(const ofbx::Mesh *mesh, const ofbx::Object *node) -> glm::mat4
		{
			auto *skin = mesh ? mesh->getGeometry()->getSkin() : nullptr;
			if (skin)
			{
				for (int i = 0, c = skin->getClusterCount(); i < c; i++)
				{
					const ofbx::Cluster *cluster = skin->getCluster(i);
					if (cluster->getLink() == node)
					{
						return toMatrix<glm::mat4>(cluster->getTransformLinkMatrix());
					}
				}
			}
			return toMatrix<glm::mat4>(node->getGlobalTransform());
		}

		inline ofbx::Vec3 operator*(const ofbx::Vec3 &vec3, float v)
		{
			return {vec3.x * v, vec3.y * v, vec3.z * v};
		}

		inline auto loadMesh(const std::string &fileName, const ofbx::IScene *scene, std::vector<const ofbx::Object *> &sceneBone, std::vector<std::shared_ptr<IResource>> &outRes, Orientation orientation)
		{
			if (scene->getMeshCount() > 0)
			{
				auto meshes = std::make_shared<MeshResource>(fileName);
				outRes.emplace_back(meshes);

				constexpr float FBXToGameUnit = 0.01f;

				for (int32_t i = 0; i < scene->getMeshCount(); ++i)
				{
					const auto fbxMesh     = (const ofbx::Mesh *) scene->getMesh(i);
					const auto geom        = fbxMesh->getGeometry();
					const auto numIndices  = geom->getIndexCount();
					const auto vertexCount = geom->getVertexCount();
					const auto vertices    = geom->getVertices();
					const auto normals     = geom->getNormals();
					const auto tangents    = geom->getTangents();
					const auto colors      = geom->getColors();
					const auto uvs         = geom->getUVs();
					const auto materials   = geom->getMaterials();

					if (numIndices == 0)
						continue;

					std::vector<std::shared_ptr<Material>> pbrMaterialsCache;

					std::vector<Vertex>        tempVertices;

					tempVertices.resize(vertexCount);

					std::vector<uint32_t> indicesArray;

					indicesArray.resize(numIndices);

					auto boundingBox = std::make_shared<BoundingBox>();

					const auto indices = geom->getFaceIndices();

					ofbx::Vec3 *generatedTangents = nullptr;
					if (!tangents && normals && uvs)
					{
						generatedTangents = new ofbx::Vec3[vertexCount];
						computeTangents(generatedTangents, vertexCount, vertices, normals, uvs);
					}

					auto transform = getTransform(fbxMesh, orientation);
					bool skin = false;

					transform.setLocalScale({FBXToGameUnit,
					                         FBXToGameUnit,
					                         FBXToGameUnit});
					transform.setWorldMatrix(glm::mat4(1.f));

					for (int32_t i = 0; i < vertexCount; ++i)
					{
						const ofbx::Vec3 &cp = vertices[i];

#define GEN_VERTEX(Vertices)                                                                                                                                 \
	{                                                                                                                                                        \
		auto &vertex = Vertices[i];                                                                                                                          \
		vertex.pos   = transform.getWorldMatrix() * glm::vec4(float(cp.x), float(cp.y), float(cp.z), 1.0);                                                   \
		fixOrientation(vertex.pos, orientation);                                                                                                             \
		boundingBox->merge(vertex.pos);                                                                                                                      \
		if (normals)                                                                                                                                         \
		{                                                                                                                                                    \
			glm::mat3 matrix(transform.getWorldMatrix());                                                                                                    \
			vertex.normal = glm::transpose(glm::inverse(matrix)) * glm::normalize(glm::vec3{float(normals[i].x), float(normals[i].y), float(normals[i].z)}); \
		}                                                                                                                                                    \
		if (uvs)                                                                                                                                             \
			vertex.texCoord = {float(uvs[i].x), float(uvs[i].y)};                                                                                     \
		if (colors)                                                                                                                                          \
			vertex.color = {float(colors[i].x), float(colors[i].y), float(colors[i].z), float(colors[i].w)};                                                 \
		else                                                                                                                                                 \
			vertex.color = {1, 1, 1, 1};                                                                                                                     \
		if (tangents)                                                                                                                                        \
			vertex.tangent = transform.getWorldMatrix() * glm::vec4(float(tangents[i].x), float(tangents[i].y), float(tangents[i].z), 1.0);                  \
		fixOrientation(vertex.normal, orientation);                                                                                                          \
		fixOrientation(vertex.tangent, orientation);                                                                                                         \
	}

						GEN_VERTEX(tempVertices);
					}

					for (int32_t i = 0; i < numIndices; i++)
					{
						int32_t index   = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];
						indicesArray[i] = index;
					}

					std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(indicesArray, tempVertices);

					for (auto i = 0; i < fbxMesh->getMaterialCount(); i++)
					{
						const ofbx::Material *material = fbxMesh->getMaterial(i);
						pbrMaterialsCache.emplace_back(loadMaterial(material, false));
					}

					std::vector<std::shared_ptr<Material>> pbrMaterials;
				

					const auto trianglesCount = vertexCount / 3;

					std::vector<uint32_t> subMeshIdx;

					if (fbxMesh->getMaterialCount() > 1)
					{
						int32_t rangeStart    = 0;
						int32_t rangeStartVal = materials[rangeStart];
						pbrMaterials.emplace_back(pbrMaterialsCache[rangeStartVal]);

						for (int32_t triangleIndex = 1; triangleIndex < trianglesCount; triangleIndex++)
						{
							if (rangeStartVal != materials[triangleIndex])
							{
								rangeStartVal = materials[triangleIndex];
								subMeshIdx.emplace_back(triangleIndex);
								pbrMaterials.emplace_back(pbrMaterialsCache[rangeStartVal]);
							}
						}

						subMeshIdx.emplace_back(trianglesCount);

						mesh->setSubMeshIndex(subMeshIdx);
						mesh->setSubMeshCount(subMeshIdx.size());

						MAPLE_ASSERT(subMeshIdx.size() == pbrMaterials.size(), "size is not same");
					}
					else
					{
						pbrMaterials = pbrMaterialsCache;
					}
					
					mesh->setMaterial(pbrMaterials);

					std::string name = fbxMesh->name;

					MAPLE_ASSERT(name != "", "name should not be null");

					mesh->setName(name);
					meshes->addMesh(name, mesh);

					if (generatedTangents)
						delete[] generatedTangents;
				}
			}
		}
	}        // namespace

	auto FBXLoader::load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &outRes) const -> void
	{
		mio::mmap_source mmap(fileName);
		MAPLE_ASSERT(mmap.is_open(), "open fbx file failed");

		constexpr bool ignoreGeometry = false;
		const uint64_t flags          = ignoreGeometry ? (uint64_t) ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t) ofbx::LoadFlags::TRIANGULATE;
		auto           scene          = ofbx::load((uint8_t *) mmap.data(), mmap.size(), flags);

		const ofbx::GlobalSettings *settings = scene->getGlobalSettings();

		Orientation orientation = Orientation::Y_UP;

		switch (settings->UpAxis)
		{
			case ofbx::UpVector_AxisX:
				orientation = Orientation::X_UP;
				break;
			case ofbx::UpVector_AxisY:
				orientation = Orientation::Y_UP;
				break;
			case ofbx::UpVector_AxisZ:
				orientation = Orientation::Z_UP;
				break;
		}
		std::vector<const ofbx::Object *> sceneBone;

		loadMesh(fileName, scene, sceneBone, outRes, orientation);
	}
};        // namespace maple
