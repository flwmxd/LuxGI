//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace maple::raytracing
{

	struct alignas(32) MaterialData
	{
		glm::ivec4 textureIndices0 = glm::ivec4(-1);        // x: albedo, y: normals, z: roughness, w: metallic
		glm::ivec4 textureIndices1 = glm::ivec4(-1);        // x: emissive, y: ao
		glm::vec4  albedo;
		glm::vec4  roughness;
		glm::vec4  metalic;
		glm::vec4  emissive;
		glm::vec4  usingValue0;        // albedo metallic roughness
		glm::vec4  usingValue1;        // normal ao emissive workflow
	};

	struct alignas(16) TransformData
	{
		glm::mat4 model;
		glm::mat4 normalMatrix;
		uint32_t  meshIndex;
		float     padding[3];
	};

}        // namespace maple::raytracing