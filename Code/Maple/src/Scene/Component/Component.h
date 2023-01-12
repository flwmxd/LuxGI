//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include "Engine/Core.h"

namespace maple
{
	namespace component
	{
		struct NameComponent
		{
			std::string name;
			SERIALIZATION(name);
		};

		struct StencilComponent
		{
		};

		struct ActiveComponent
		{
			bool active = true;

			SERIALIZATION(active);
		};
	}        // namespace component

	namespace global::component
	{
		struct DeltaTime
		{
			float dt;
		};
	}        // namespace global::component
};           // namespace maple
