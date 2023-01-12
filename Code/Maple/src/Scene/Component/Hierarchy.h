//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <entt.hpp>
#include "Engine/Core.h"

namespace maple
{
	namespace component
	{
		struct Hierarchy
		{
			entt::entity parent = entt::null;
			entt::entity first  = entt::null;
			entt::entity next   = entt::null;
			entt::entity prev   = entt::null;

			SERIALIZATION(parent,first,next,prev);
		};
	}        // namespace component
};           // namespace maple
