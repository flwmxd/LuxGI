//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <string>

namespace maple
{
	namespace component
	{
		struct Environment;
	}

	namespace environment
	{
		auto MAPLE_EXPORT init(component::Environment &, const std::string &fileName) -> void;
	};
};        // namespace maple