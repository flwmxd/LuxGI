//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Math/BoundingBox.h"
#include <glm/glm.hpp>

namespace maple
{
	namespace component
	{
		struct BoundingBoxComponent
		{
			BoundingBox box = {{}, {}};
			SERIALIZATION(box);
		};
	}        // namespace component
};           // namespace maple
