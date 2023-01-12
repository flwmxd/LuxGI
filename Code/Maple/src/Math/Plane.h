//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <cstdint>
#include <glm/glm.hpp>
namespace maple
{
	class Plane final
	{
	  public:
		inline constexpr Plane() :
		    normal(0.0f, 1.0f, 0.0f),
		    distance(0)
		{
		}

		inline Plane(const glm::vec3 &normal, float distance) :
		    normal(normal),
		    distance(distance)
		{
		}

		inline Plane(const glm::vec3 &point, const glm::vec3 &n)
		{
			normal   = glm::normalize(n);
			distance = glm::dot(normal, point);
		}

		inline Plane(const glm::vec3 &point1, const glm::vec3 &point2, const glm::vec3 &point3)
		{
			glm::vec3 edge1 = point2 - point1;
			glm::vec3 edge2 = point3 - point1;
			normal          = glm::normalize(glm::cross(edge1, edge2));
			distance        = glm::dot(normal, point1);
		}
		inline Plane(const glm::vec4 &plane)
		{
			normal   = glm::vec3(plane.x, plane.y, plane.z);
			distance = plane.w;
		}

		inline Plane(float a, float b, float c, float d)
		{
			normal   = glm::vec3(a, b, c);
			distance = d;
		}

		inline auto set(const glm::vec3 &normal, float distance)
		{
			this->normal   = normal;
			this->distance = distance;
		}
		inline auto set(const glm::vec3 &point, const glm::vec3 &normal)
		{
			this->normal   = normal;
			this->distance = glm::dot(normal, point);
		}

		inline auto set(const glm::vec3 &point1, const glm::vec3 &point2, const glm::vec3 &point3)
		{
			glm::vec3 vec1 = point2 - point1;
			glm::vec3 vec2 = point3 - point1;
			normal         = glm::cross(vec1, vec2);
			normal         = glm::normalize(normal);
			distance       = glm::dot(normal, point1);
		}

		inline auto set(const glm::vec4 &plane)
		{
			normal   = glm::vec3(plane.x, plane.y, plane.z);
			distance = plane.w;
		}

		inline auto setNormal(const glm::vec3 &normal)
		{
			this->normal = normal;
		}

		inline auto setDistance(float distance)
		{
			this->distance = distance;
		}

		inline auto getDistance(const glm::vec3 &point) const
		{
			return glm::dot(point, normal) + distance;
		}
		inline auto getDistance(const glm::vec4 &point) const
		{
			return glm::dot(glm::vec3(point), normal) + distance;
		}

		inline auto isOnPlane(const glm::vec3 &point) const
		{
			return getDistance(point) >= -0.0001f;
		}
		inline auto isOnPlane(const glm::vec4 &point) const
		{
			return getDistance(point) >= -0.0001f;
		}

		inline auto transform(const glm::mat4 &matrix)

		{
			glm::vec4 plane = glm::vec4(normal, distance);
			plane           = matrix * plane;
			normal          = glm::vec3(plane.x, plane.y, plane.z);
			distance        = plane.w;
		}

		inline auto transformed(const glm::mat4 &matrix) const
		{
			glm::vec4 plane = glm::vec4(normal, distance);
			plane           = matrix * plane;
			return Plane(glm::vec3(plane.x, plane.y, plane.z), plane.w);
		}

		inline auto normalize()
		{
			float magnitude = glm::length(normal);
			normal /= magnitude;
			distance /= magnitude;
		}

		inline auto project(const glm::vec3 &point) const
		{
			return point - getDistance(point) * normal;
		}

		inline auto getNormal() const
		{
			return normal;
		}
		inline float getDistance() const
		{
			return distance;
		}

		inline operator glm::vec4() const
		{
			return {normal, distance};
		}

	  private:
		glm::vec3 normal;
		float     distance;
	};
};        // namespace maple
