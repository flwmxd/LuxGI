//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Math/BoundingBox.h"

#include <memory>

namespace maple
{
	class Mesh;
	class Texture3D;
	class CommandBuffer;

	namespace component
	{
		class Transform;
	}

	namespace sdf::component
	{
		struct MeshDistanceField;
	}
	namespace sdf::baker
	{
		struct SDFBakerConfig
		{
			constexpr SDFBakerConfig(uint32_t maxResolution = 128, uint32_t minResolution = 32, float targetTexelPerMeter = 3, int32_t sampleCount = 4, bool buildWithBVH = true) :
			    maxResolution(maxResolution), minResolution(minResolution), targetTexelPerMeter(targetTexelPerMeter), sampleCount(sampleCount), buildWithBVH(buildWithBVH){};

			uint32_t maxResolution;
			uint32_t minResolution;
			float    targetTexelPerMeter;
			int32_t  sampleCount;
			bool     buildWithBVH;
		};
		
		inline auto paddingSDFBox(const BoundingBox &bb) -> BoundingBox
		{
			glm::vec3       padding    = 0.05f * bb.size();
			constexpr float minPadding = 0.1f;
			padding                    = glm::max(padding, glm::vec3(minPadding));
			return {bb.min - padding, bb.max + padding};
		}

		/**
		 * TODO ...can also use GPU to generate
		 */

		auto MAPLE_EXPORT load(component::MeshDistanceField &field, const CommandBuffer *cmd) -> void;
		auto MAPLE_EXPORT bake(const std::shared_ptr<Mesh> &mesh, const SDFBakerConfig &config, component::MeshDistanceField &field, const maple::component::Transform &transform) -> void;
	};        // namespace sdf::baker
}        // namespace maple