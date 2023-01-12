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
	class BoundingSphere;

	class MAPLE_EXPORT BoundingBox
	{
	  public:
		BoundingBox() :
		    min(INFINITY, INFINITY, INFINITY),
		    max(-INFINITY, -INFINITY, -INFINITY)
		{}
		BoundingBox(const glm::vec3 &min, const glm::vec3 &max) :
		    min(min),
		    max(max)
		{}
		BoundingBox(const Rect2D &rect) :
		    min(rect.min, 0.0f),
		    max(rect.max, 0.0f)
		{}
		inline auto operator==(const BoundingBox &rhs) const
		{
			return (min == rhs.min && max == rhs.max);
		}
		inline auto operator!=(const BoundingBox &rhs) const
		{
			return (min != rhs.min || max != rhs.max);
		}

		inline auto merge(const glm::vec3 &point) -> void
		{
			if (point.x < min.x)
				min.x = point.x;
			if (point.y < min.y)
				min.y = point.y;
			if (point.z < min.z)
				min.z = point.z;
			if (point.x > max.x)
				max.x = point.x;
			if (point.y > max.y)
				max.y = point.y;
			if (point.z > max.z)
				max.z = point.z;
		}

		inline auto merge(const BoundingBox &box) -> void
		{
			if (box.min.x < min.x)
				min.x = box.min.x;
			if (box.min.y < min.y)
				min.y = box.min.y;
			if (box.min.z < min.z)
				min.z = box.min.z;
			if (box.max.x > max.x)
				max.x = box.max.x;
			if (box.max.y > max.y)
				max.y = box.max.y;
			if (box.max.z > max.z)
				max.z = box.max.z;
		}

		inline auto merge(const std::shared_ptr<BoundingBox> &box) -> void
		{
			if (box->min.x < min.x)
				min.x = box->min.x;
			if (box->min.y < min.y)
				min.y = box->min.y;
			if (box->min.z < min.z)
				min.z = box->min.z;
			if (box->max.x > max.x)
				max.x = box->max.x;
			if (box->max.y > max.y)
				max.y = box->max.y;
			if (box->max.z > max.z)
				max.z = box->max.z;
		}

		auto transform(const glm::mat4 &transform) const -> BoundingBox;

		inline auto clear() -> void
		{
			min = {INFINITY, INFINITY, INFINITY};
			max = {-INFINITY, -INFINITY, -INFINITY};
		}

		inline auto center() const
		{
			return (max + min) * 0.5f;
		}
		inline auto size() const
		{
			return max - min;
		}

		inline auto isDefined() const
		{
			return min.x != INFINITY;
		}

		inline auto contains(const glm::vec3 &point) const
		{
			return !(point.x < min.x || point.x > max.x || point.y < min.y || point.y > max.y || point.z < min.z || point.z > max.z);
		}
		
		auto intersects(const BoundingBox &box) const -> bool;
		auto intersectsWithSphere(const BoundingSphere &box) const -> bool;

		glm::vec3 min;
		glm::vec3 max;

		SERIALIZATION(min, max);
	};

};        // namespace maple