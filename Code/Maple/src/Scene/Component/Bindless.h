//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>


namespace maple
{
	class Material;

	namespace global::component
	{
		struct Bindless
		{
			std::unordered_map<uint32_t, uint32_t> meshIndices;
			std::unordered_map<uint32_t, uint32_t> materialIndices;
			std::unordered_map<uint32_t, uint32_t> textureIndices;
		};

		struct MaterialChanged
		{
			std::vector<Material *> updateQueue;
		};
	}        // namespace global::component
}        // namespace maple