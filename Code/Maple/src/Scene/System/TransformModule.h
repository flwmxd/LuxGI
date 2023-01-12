//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"

namespace maple
{
	class SystemBuilder;

	namespace component
	{
		class Transform;
	}

	namespace transform
	{
		auto registerTransformModule(std::shared_ptr<SystemBuilder> builder) -> void;
	}        // namespace hierarchy
};           // namespace maple
