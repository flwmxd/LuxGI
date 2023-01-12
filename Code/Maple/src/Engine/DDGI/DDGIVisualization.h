//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "IoC/SystemBuilder.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct DDGIVisualization
			{
				bool enable = true;
				float scale  = 1.f;
			};
		};        // namespace component

		auto registerDDGIVisualization(SystemQueue &begin, SystemQueue &render, std::shared_ptr<SystemBuilder> point) -> void;
	};        // namespace ddgi
}        // namespace maple