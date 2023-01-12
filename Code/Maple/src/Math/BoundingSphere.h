//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include "Engine/Core.h"
#include "Rect2D.h"

namespace maple
{
	class BoundingBox;

	class BoundingSphere
	{
	  public:
		BoundingSphere(const glm::vec3 &center, float radius) :
		    center(center), radius(radius)
		{}

		BoundingSphere(const BoundingBox &box);

		glm::vec3 center{0};
		float     radius{0.f};
	};

};        // namespace maple