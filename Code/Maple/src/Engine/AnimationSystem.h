//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"

#include <memory>
#include <string>
#include <vector>

namespace maple
{
	namespace animation
	{
		namespace component
		{
			struct Animator
			{
				std::string blueprintPath;
				SERIALIZATION(blueprintPath);
			};

			struct SkeletonComponent
			{
				bool drawBones  = false;
				bool drawJoints = false;
			};
		}        // namespace component
	}        // namespace animation
}        // namespace maple