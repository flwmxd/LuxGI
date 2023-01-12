//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
namespace maple
{
	struct Vertex
	{
		glm::vec3   pos;
		glm::vec4   color;
		glm::vec2   texCoord;
		glm::vec3   normal;
		glm::vec3   tangent;

		inline auto operator-(const Vertex &right) -> Vertex
		{
			Vertex ret;
			ret.pos      = pos - right.pos;
			ret.color    = color - right.color;
			ret.texCoord = texCoord - right.texCoord;
			ret.normal   = normal - right.normal;
			return ret;
		}

		inline auto operator+(const Vertex &right) -> Vertex
		{
			Vertex ret;
			ret.pos      = pos + right.pos;
			ret.color    = color + right.color;
			ret.texCoord = texCoord + right.texCoord;
			ret.normal   = normal + right.normal;
			return ret;
		}

		inline auto operator*(float factor) -> Vertex
		{
			Vertex ret;
			ret.pos      = pos * factor;
			ret.color    = color * factor;
			ret.texCoord = texCoord * factor;
			ret.normal   = normal * factor;
			return ret;
		}

		inline auto operator==(const Vertex &other) const -> bool
		{
			return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal && tangent == other.tangent;//
		}
	};

	struct Vertex2D
	{
		glm::vec3   vertex;
		glm::vec4   color;
		glm::vec3   uv;
		inline auto operator==(const Vertex2D &other) const -> bool
		{
			return vertex == other.vertex && uv == other.uv && color == other.color;
		}
	};

	struct LineVertex
	{
		glm::vec3 vertex;
		glm::vec4 color;

		inline auto operator==(const LineVertex &other) const
		{
			return vertex == other.vertex && color == other.color;
		}
	};

	struct PointVertex
	{
		glm::vec3 vertex;
		glm::vec4 color;
		glm::vec2 size;
		glm::vec2 uv;

		inline auto operator==(const PointVertex &other) const
		{
			return vertex == other.vertex && color == other.color && size == other.size && uv == other.uv;
		}
	};
};        // namespace maple
namespace std
{
	template <>
	struct hash<maple::Vertex>
	{
		size_t operator()(const maple::Vertex &vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
			         (hash<glm::vec2>()(vertex.texCoord) << 1) ^
			         (hash<glm::vec4>()(vertex.color) << 1) ^
			         (hash<glm::vec3>()(vertex.normal) << 1)));
		}
	};
}        // namespace std