//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Frustum.h"
#include "BoundingBox.h"
#include "Engine/Profiler.h"
#include <glm/geometric.hpp>

namespace maple
{
	const glm::vec3 frustumCorners[8] = {
	    {1.0f, 1.0f, -1.0f},
	    {1.0f, -1.0f, -1.0f},
	    {-1.0f, -1.0f, -1.0f},
	    {-1.0f, 1.0f, -1.0f},
	    {1.0f, 1.0f, 1.0f},
	    {1.0f, -1.0f, 1.0f},
	    {-1.0f, -1.0f, 1.0f},
	    {-1.0f, 1.0f, 1.0f}};

	auto Frustum::from(const glm::mat4 &m) -> void
	{
		PROFILE_FUNCTION();
		auto projInverse = glm::inverse(m);

		planes[PlaneLeft]  = Plane(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
		planes[PlaneRight] = Plane(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
		planes[PlaneDown]  = Plane(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
		planes[PlaneUp]    = Plane(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
		planes[PlaneNear]  = Plane(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
		planes[PlaneFar]   = Plane(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);

		for (int i = 0; i < 6; i++)
		{
			planes[i].normalize();
		}

		for (uint32_t j = 0; j < 8; j++)
		{
			auto invCorner = projInverse * glm::vec4(frustumCorners[j], 1.0f);
			vertices[j]    = invCorner / invCorner.w;
		}
	}

	auto Frustum::isInside(const glm::vec3 &pos) const -> bool
	{
		PROFILE_FUNCTION();
		for (int32_t i = 0; i < 6; i++)
		{
			if (planes[i].getDistance(pos) < 0.0f)
			{
				return false;
			}
		}
		return true;
	}

	auto Frustum::isInside(const BoundingBox &box) const -> bool
	{
		PROFILE_FUNCTION();
		for (int i = 0; i < 6; i++)
		{
			glm::vec3 p = box.min;
			glm::vec3 n = box.max;
			glm::vec3 N = planes[i].getNormal();
			if (N.x >= 0)
			{
				p.x = box.max.x;
				n.x = box.min.x;
			}
			if (N.y >= 0)
			{
				p.y = box.max.y;
				n.y = box.min.y;
			}
			if (N.z >= 0)
			{
				p.z = box.max.z;
				n.z = box.min.z;
			}
			if (planes[i].getDistance(p) < 0)
			{
				return false;
			}
		}
		return true;
	}

	auto Frustum::isInside(const std::shared_ptr<BoundingBox> &box) const -> bool
	{
		PROFILE_FUNCTION();
		for (int i = 0; i < 6; i++)
		{
			glm::vec3 p = box->min;
			glm::vec3 n = box->max;
			glm::vec3 N = planes[i].getNormal();
			if (N.x >= 0)
			{
				p.x = box->max.x;
				n.x = box->min.x;
			}
			if (N.y >= 0)
			{
				p.y = box->max.y;
				n.y = box->min.y;
			}
			if (N.z >= 0)
			{
				p.z = box->max.z;
				n.z = box->min.z;
			}
			if (planes[i].getDistance(p) < 0)
			{
				return false;
			}
		}
		return true;
	}

};        // namespace maple
