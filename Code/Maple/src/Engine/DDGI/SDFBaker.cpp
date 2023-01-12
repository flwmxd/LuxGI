//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SDFBaker.h"
#include "MeshDistanceField.h"

#include "Engine/Core.h"
#include "Engine/Mesh.h"
#include "Math/MathUtils.h"
#include "Others/Console.h"
#include "Others/Timer.h"
#include "RHI/CommandBuffer.h"
#include "RHI/Texture.h"
#include "Scene/Component/Transform.h"
#include <bvh_tree.h>
#include <glm/gtc/packing.hpp>

#include "IO/Serialization.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

namespace maple
{
	namespace sdf::baker
	{
		inline auto getMipLevels(int32_t width, int32_t height, int32_t depth)
		{
			int32_t result = 1;
			while (width > 1 || height > 1 || depth > 1)
			{
				if (width > 1)
					width >>= 1;
				if (height > 1)
					height >>= 1;
				if (depth > 1)
					depth >>= 1;
				result++;
			}
			return result;
		}

		/**
		 *  flatten to one dimension.
		 */
		inline auto flatten(const glm::ivec3 &position, const glm::ivec3 &resolution)
		{
			return position.x + position.y * resolution.x + position.z * resolution.x * resolution.y;
		};

		auto bake(const std::shared_ptr<Mesh> &mesh, const SDFBakerConfig &config, component::MeshDistanceField &field, const maple::component::Transform &transform) -> void
		{
			auto meshAABB = mesh->getBoundingBox();

			glm::uvec3 sdfSize{};
			const auto bbExtents = meshAABB->size();
			for (int32_t component = 0; component < 3; component++)
			{
				float targetRes    = bbExtents[component] / config.targetTexelPerMeter;
				//targetRes          = math::nextPowerOfTwo((uint32_t) targetRes);
				sdfSize[component] = glm::clamp(targetRes, (float) config.minResolution, (float) config.maxResolution);
			}

			std::vector<glm::vec3> positions;
			positions.resize(mesh->getVertex().size());

			std::transform(mesh->getVertex().begin(), mesh->getVertex().end(), positions.begin(), [](const auto &v) {
				return v.pos;
			});

			BoundingBox paddingAABB = paddingSDFBox(*meshAABB);

			field.aabb          = paddingAABB;
			field.localToUVWMul = 1.f / paddingAABB.size();
			field.localToUVWAdd = -paddingAABB.min / paddingAABB.size();
			field.maxDistance   = math::max3(paddingAABB.size());

			acc::BVHTree<uint32_t, glm::vec3> bvh(mesh->getIndex(), positions, 1);

			std::vector<glm::vec3> sampleDirections;

			for (int32_t sampleIndexX = 0; sampleIndexX < config.sampleCount; sampleIndexX++)
			{
				for (int32_t sampleIndexY = 0; sampleIndexY < config.sampleCount; sampleIndexY++)
				{
					float sampleX = sampleIndexX / float(config.sampleCount - 1);                //0 - 1
					float sampleY = sampleIndexY / float(config.sampleCount - 1) * 2 - 1;        //-1 - 1
					//0-2pi
					const float phi   = sampleX * M_PI_TWO;
					const float theta = std::acosf(sampleY);
					sampleDirections.emplace_back(math::directionToVector({phi, theta}));
				}
			}

			const uint32_t       pixelCount = sdfSize.x * sdfSize.y * sdfSize.z;
			const uint32_t       pixelSize  = sizeof(float) / 2;        //stored as 16 bit float
			std::vector<uint8_t> byteData(pixelCount * pixelSize, 0);

			for (auto z = 0; z < sdfSize.z; z++)
			{
				for (auto y = 0; y < sdfSize.y; y++)
				{
					for (auto x = 0; x < sdfSize.x; x++)
					{
						float minDistance = std::numeric_limits<float>::infinity();
						// math::max3(sdfSize);

						auto sizeOfGrid = paddingAABB.size() / glm::vec3(sdfSize - glm::uvec3(1));

						glm::vec3 voxelPos = glm::vec3((float) x, (float) y, (float) z) * sizeOfGrid + paddingAABB.min;

						int32_t hitBackCount = 0, hitCount = 0;

						minDistance = glm::distance(bvh.closest_point(voxelPos, minDistance), voxelPos);

						for (int32_t sample = 0; sample < sampleDirections.size(); sample++)
						{
							acc::Ray<glm::vec3>                    ray{voxelPos, sampleDirections[sample], 0.00000001, field.maxDistance};
							acc::BVHTree<uint32_t, glm::vec3>::Hit hits;

							if (bvh.intersect(ray, &hits))
							{
								auto i0 = mesh->getIndex()[hits.idx * 3];
								auto i1 = mesh->getIndex()[hits.idx * 3 + 1];
								auto i2 = mesh->getIndex()[hits.idx * 3 + 2];
								auto v0 = mesh->getVertex()[i0].pos;
								auto v1 = mesh->getVertex()[i1].pos;
								auto v2 = mesh->getVertex()[i2].pos;

								auto normal = glm::normalize(glm::cross(v0 - v2, v0 - v1));

								hitCount++;
								const bool backHit = glm::dot(ray.dir, normal) > 0;
								if (backHit)
									hitBackCount++;
							}
						}

						float distance = (float) minDistance;
						if ((float) hitBackCount > (float) sampleDirections.size() * 0.5 && hitCount != 0)
						{
							distance *= -1;
						}

						distance /= field.maxDistance;
						distance                 = (distance + 1.) / 2.f;
						const uint32_t index     = flatten(glm::ivec3(x, y, z), glm::ivec3(sdfSize));
						const uint32_t byteIndex = index * pixelSize;

						uint16_t half                   = glm::packHalf(glm::vec1(distance))[0];        //distance is stored as 16 bit float
						byteData[byteIndex]             = ((uint8_t *) &half)[0];
						byteData[size_t(byteIndex) + 1] = ((uint8_t *) &half)[1];
					}
				}
			}

			const int32_t mipCount = std::min(getMipLevels(sdfSize.x, sdfSize.y, sdfSize.z), 3);

			Timer timer;
			field.bakedPath = "sdf/" + mesh->getName() + "_" + std::to_string(timer.currentTimestamp()) + ".sdf";
			std::ofstream               os(field.bakedPath, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);
			archive(sdfSize, mipCount, byteData);

			for (int32_t mipLevel = 1; mipLevel < mipCount; mipLevel++)
			{
				glm::ivec3           resolutionMip = glm::max(sdfSize / 2u, glm::uvec3(1));
				const auto           voxelsMipSize = resolutionMip.x * resolutionMip.y * resolutionMip.z * pixelSize;
				std::vector<uint8_t> voxelData(voxelsMipSize, 0);

				for (uint32_t z = 0; z < resolutionMip.z; z++)
				{
					for (uint32_t y = 0; y < resolutionMip.y; y++)
					{
						for (uint32_t x = 0; x < resolutionMip.x; x++)
						{
							float distance = 0;
							for (uint32_t dz = 0; dz < 2; dz++)
							{
								for (uint32_t dy = 0; dy < 2; dy++)
								{
									for (uint32_t dx = 0; dx < 2; dx++)
									{
										auto     index = flatten({x * 2 + dx, y * 2 + dy, z * 2 + dz}, sdfSize) * pixelSize;
										uint16_t value;
										memcpy(&value, &byteData[index], sizeof(uint16_t));
										distance += glm::unpackHalf1x16(value);
									}
								}
							}
							distance *= 1.0f / 8.0f;
							auto     index               = flatten({x, y, z}, resolutionMip) * pixelSize;
							uint16_t half                = glm::packHalf(glm::vec1(distance))[0];        //distance is stored as 16 bit float
							voxelData[index]             = ((uint8_t *) &half)[0];
							voxelData[size_t(index) + 1] = ((uint8_t *) &half)[1];
						}
					}
				}

				archive(voxelData);
				sdfSize  = resolutionMip;
				byteData = voxelData;
			}
		}

		auto load(component::MeshDistanceField &field, const CommandBuffer *cmd) -> void
		{
			const CommandBuffer *cmd1 = cmd;
			CommandBuffer::Ptr   onceCmd;
			if (cmd == nullptr)
			{
				onceCmd = CommandBuffer::create();
				onceCmd->init(true);
				onceCmd->beginRecording();
				cmd1 = onceCmd.get();
			}

			std::ifstream              os(field.bakedPath, std::ios::binary);
			cereal::BinaryInputArchive archive(os);
			glm::uvec3                 sdfSize{};
			int32_t                    mipCount;
			std::vector<uint8_t>       data;
			archive(sdfSize, mipCount, data);
			auto sdf3d = Texture3D::create(sdfSize.x, sdfSize.y, sdfSize.z, {TextureFormat::R16, TextureFilter::Linear}, {false, false, true, false, mipCount});
			sdf3d->update(cmd1, data.data());

			for (int32_t mipLevel = 1; mipLevel < mipCount; mipLevel++)
			{
				std::vector<uint8_t> voxelData;
				archive(voxelData);
				sdf3d->updateMipmap(cmd1, mipLevel, voxelData.data());
			}

			if (cmd == nullptr)
			{
				onceCmd->endSingleTimeCommands();
			}
			field.buffer = sdf3d;
		}

	};        // namespace sdf::baker
};            // namespace maple
