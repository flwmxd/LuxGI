//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "OrientedBoundingBox.h"
#include "BoundingBox.h"

namespace maple
{
	OrientedBoundingBox::OrientedBoundingBox(const BoundingBox &bb)
	{
		const auto center = bb.center();
		extends           = bb.max - center;
		transform         = glm::translate(glm::mat4(1), center);
	}

	auto OrientedBoundingBox::setTransform(const glm::mat4 &worldMatrix) -> void
	{
		transform = worldMatrix * transform;
	}

	auto OrientedBoundingBox::getSize() const -> glm::vec3
	{
		const auto full = extends * 2.f;
		const auto xv   = transform * glm::vec4(full.x, 0, 0, 1);
		const auto yv   = transform * glm::vec4(0, full.y, 0, 1);
		const auto zv   = transform * glm::vec4(0, 0, full.z, 1);
		return {glm::length(xv), glm::length(yv), glm::length(zv)};
	}
}        // namespace maple