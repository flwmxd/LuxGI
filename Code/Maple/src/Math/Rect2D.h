//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
namespace maple
{
	class MAPLE_EXPORT Rect2D final
	{
	  public:
		Rect2D() :
		    min(INFINITY, INFINITY),
		    max(-INFINITY, -INFINITY)
		{}

		Rect2D(const glm::vec2 &min, const glm::vec2 &max) :
		    min(min),
		    max(max)
		{
		}

		Rect2D(float left, float top, float right, float bottom) :
		    min(left, top),
		    max(right, bottom)
		{
		}

		inline auto operator==(const Rect2D &rhs) const
		{
			return min == rhs.min && max == rhs.max;
		}
		inline auto operator!=(const Rect2D &rhs) const
		{
			return min != rhs.min || max != rhs.max;
		}

		inline auto &operator+=(const Rect2D &rhs)
		{
			min += rhs.min;
			max += rhs.max;
			return *this;
		}

		inline auto &operator-=(const Rect2D &rhs)
		{
			min -= rhs.min;
			max -= rhs.max;
			return *this;
		}

		inline auto &operator/=(float value)
		{
			min /= value;
			max /= value;
			return *this;
		}

		inline auto &operator*=(float value)
		{
			min *= value;
			max *= value;
			return *this;
		}

		inline auto operator/(float value) const -> Rect2D
		{
			return {min / value, max / value};
		}

		inline auto operator*(float value) const -> Rect2D
		{
			return {min * value, max * value};
		}

		inline auto operator+(const Rect2D &rhs) const -> Rect2D
		{
			return {min + rhs.min, max + rhs.max};
		}

		inline auto operator-(const Rect2D &rhs) const -> Rect2D
		{
			return {min - rhs.min, max - rhs.max};
		}

		inline auto merge(const glm::vec2 &point)
		{
			if (point.x < min.x)
				min.x = point.x;
			if (point.x > max.x)
				max.x = point.x;
			if (point.y < min.y)
				min.y = point.y;
			if (point.y > max.y)
				max.y = point.y;
		}

		inline auto merge(const Rect2D &rect)
		{
			if (rect.min.x < min.x)
				min.x = rect.min.x;
			if (rect.min.y < min.y)
				min.y = rect.min.y;
			if (rect.max.x > max.x)
				max.x = rect.max.x;
			if (rect.max.y > max.y)
				max.y = rect.max.y;
		}

		inline auto clear()
		{
			min = {INFINITY, INFINITY};
			max = {-INFINITY, -INFINITY};
		}

		inline auto center() const
		{
			return (max + min) * 0.5f;
		}
		inline auto size() const
		{
			return max - min;
		}

		inline auto contains(const glm::vec2 &point) const
		{
			return !(point.x < min.x || point.y < min.y || point.x > max.x || point.y > max.y);
		}

		inline auto getLeft() const
		{
			return min.x;
		}
		inline auto getTop() const
		{
			return min.y;
		}
		inline auto getRight() const
		{
			return max.x;
		}
		inline auto getBottom() const
		{
			return max.y;
		}

		glm::vec2 min;
		glm::vec2 max;
	};
};        // namespace maple