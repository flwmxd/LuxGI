//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <string>

namespace maple
{
	namespace physics
	{
		enum class ColliderType : int32_t
		{
			BoxCollider,
			SphereCollider,
			CapsuleCollider,
			TriangleMeshCollider,
			ClothCollider,
			Length
		};

		static const char *ColliderTypeNames[] =
		    {
		        "BoxCollider",
		        "SphereCollider",
		        "CapsuleCollider",
		        "TriangleMeshCollider",
		        "ClothCollider",nullptr};

		inline auto getNameByType(ColliderType type)
		{
			return ColliderTypeNames[static_cast<int32_t>(type)];
		}
	}        // namespace physics
}        // namespace maple