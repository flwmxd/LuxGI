//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <memory>
#include <string>

namespace maple
{
	class CameraController;

	namespace camera_controller
	{
		enum class ControllerType : int32_t
		{
			FPS = 0,
			ThirdPerson,
			EditorCamera,
			Custom
		};

		static const char *ControllerTypeName[] = {
		    "FPS",
		    "ThirdPerson",
		    "EditorCamera"};

		namespace component
		{
			struct CameraControllerComponent
			{
				ControllerType type = ControllerType::FPS;

				SERIALIZATION(type);
			};
		}        // namespace component
	}            // namespace camera_controller
};               // namespace maple
