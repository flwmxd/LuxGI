//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Collider.h"
#include "Engine/Core.h"
#include "Engine/Vertex.h"
#include "Math/BoundingBox.h"
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	namespace physics
	{
		namespace component
		{
			struct RigidBody
			{
				bool  kinematic     = false;
				bool  dynamic       = true;
				bool  ignoreInertia = false;
				float mass          = 1.0;

				ColliderType type;

				BoundingBox aabb;
				glm::vec3   center;
				float       radius = 0.5f;
				float       height = 1.f;

				SERIALIZATION(kinematic, dynamic, ignoreInertia, mass, type, center, radius, height, aabb);
			};
		}        // namespace component
	}            // namespace physics
}        // namespace maple