//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/BoundingBox.h"
#include "SDFBaker.h"
#include <memory>

namespace maple
{
	class Texture3D;

	namespace sdf::component
	{
		struct MeshDistanceField
		{
			std::shared_ptr<Texture3D> buffer;

			std::string bakedPath;
			BoundingBox aabb;//padded...
			glm::vec3   localToUVWAdd;        // -min / size
			glm::vec3   localToUVWMul;        // 1 / size
			float       maxDistance;

			SERIALIZATION(bakedPath,aabb,localToUVWAdd,localToUVWMul,maxDistance);
		};
	}        // namespace sdf::component
}        // namespace maple