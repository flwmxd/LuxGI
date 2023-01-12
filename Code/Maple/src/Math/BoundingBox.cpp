//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "BoundingBox.h"
#include "BoundingSphere.h"
#include <glm/gtx/norm.hpp> 

namespace maple
{
	auto BoundingBox::transform(const glm::mat4 &transform) const -> BoundingBox
	{
		auto newCenter = glm::vec3(transform * glm::vec4(center(), 1.f));

		auto oldEdge = size() * 0.5f;

		auto newEdge = glm::vec3(
		    std::abs(transform[0][0]) * oldEdge.x + std::abs(transform[1][0]) * oldEdge.y + std::abs(transform[2][0] * oldEdge.z),
		    std::abs(transform[0][1]) * oldEdge.x + std::abs(transform[1][1]) * oldEdge.y + std::abs(transform[2][1] * oldEdge.z),
		    std::abs(transform[0][2]) * oldEdge.x + std::abs(transform[1][2]) * oldEdge.y + std::abs(transform[2][2] * oldEdge.z));

		return BoundingBox(newCenter - newEdge, newCenter + newEdge);
	}

	auto BoundingBox::intersects(const BoundingBox &box) const -> bool
	{
		if (box.max.x < min.x || box.min.x > max.x || box.max.y < min.y || box.min.y > max.y || box.max.z < min.z || box.min.z > max.z)
			return false;
		return true;
	}

	auto BoundingBox::intersectsWithSphere(const BoundingSphere &sphere) const -> bool
	{
		glm::vec3 vector = glm::clamp(sphere.center, min, max);
		const auto distance =  glm::distance2(sphere.center, vector);
		return distance <= sphere.radius * sphere.radius;
	}
};        // namespace maple
