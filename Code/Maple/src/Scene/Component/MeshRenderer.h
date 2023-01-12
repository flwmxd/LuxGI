//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "IO/MeshResource.h"
#include "Engine/Core.h"
#include "Engine/Material.h"
#include <cereal/types/memory.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Mesh;
	class MeshResource;

	namespace animation
	{
		class Skeleton;
	}

	namespace component
	{
		class Transform;
		enum class PrimitiveType : int32_t
		{
			Plane    = 0,
			Quad     = 1,
			Cube     = 2,
			Pyramid  = 3,
			Sphere   = 4,
			Capsule  = 5,
			Cylinder = 6,
			Terrain  = 7,
			File     = 8,
			Length
		};

		struct BoneComponent
		{
			int32_t boneIndex;
			SERIALIZATION(boneIndex);
		};

		struct MeshRenderer
		{
			bool                  castShadow = true;
			bool                  active     = true;
			PrimitiveType         type;
			std::shared_ptr<Mesh> mesh;
			std::string           meshName;
			std::string           filePath;
		};

	
		struct SkinnedMeshRenderer
		{
			bool        castShadow = true;
			std::string meshName;
			std::string filePath;

			std::shared_ptr<Mesh>                       mesh;
			std::shared_ptr<glm::mat4[]>                boneTransforms;
		};
	}        // namespace component
};           // namespace maple
