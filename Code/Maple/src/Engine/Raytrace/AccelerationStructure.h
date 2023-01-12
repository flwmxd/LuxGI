//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "IoC/SystemBuilder.h"
#include "Engine/Core.h"

namespace maple
{
	class AccelerationStructure;

	namespace raytracing
	{
		namespace global::component
		{
			struct TopLevelAs
			{
				std::shared_ptr<AccelerationStructure> topLevelAs;
			};
		}

		auto registerAccelerationStructureModule(SystemQueue &begin, std::shared_ptr<SystemBuilder> point) -> void;
	}
}        // namespace maple