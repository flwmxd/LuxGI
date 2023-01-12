//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "Math/BoundingBox.h"

namespace maple
{
	class SystemBuilder;

	namespace mesh
	{
		namespace global::component
		{
			struct SceneAABB
			{
				BoundingBox aabb;
			};
		}

		auto registerMeshModule(std::shared_ptr<SystemBuilder> builder) -> void;
	}        // namespace hierarchy
};           // namespace maple
