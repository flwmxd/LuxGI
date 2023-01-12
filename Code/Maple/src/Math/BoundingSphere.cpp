//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "BoundingSphere.h"
#include "BoundingBox.h"

namespace maple
{
	BoundingSphere::BoundingSphere(const BoundingBox &box)
	{
		center = box.center();
		radius = glm::distance(box.max, box.min) / 2.f;
	}
};        // namespace maple
