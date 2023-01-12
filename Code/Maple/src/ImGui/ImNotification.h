//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <string>

namespace maple
{
	namespace ImNotification
	{
		enum class Type
		{
			None,
			Success,
			Warning,
			Error,
			Info
		};

		auto MAPLE_EXPORT onImGui() -> void;
		auto MAPLE_EXPORT makeNotification(const std::string &title, const std::string &str, const Type type, int32_t time = 3000) -> void;
	};        // namespace ImNotification
};            // namespace maple
