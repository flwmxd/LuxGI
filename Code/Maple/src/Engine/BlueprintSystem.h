//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace maple::blueprint
{
	namespace component
	{
		enum class BlueprintState
		{
			InActive,
			Execute
		};

		struct BlueprintComponent
		{
			BlueprintState state = BlueprintState::InActive; 
		};
	}        // namespace component
}        // namespace maple::blueprint