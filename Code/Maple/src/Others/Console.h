//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdarg.h>
#include <string>

namespace maple
{
	class MAPLE_EXPORT Console
	{
	  public:
		static auto  init() -> void;
		static auto &getLogger()
		{
			return logger;
		}

	  private:
		static std::shared_ptr<spdlog::logger> logger;
	};
};        // namespace maple

#define LOGV(...) maple::Console::getLogger()->trace(__VA_ARGS__)
#define LOGI(...) maple::Console::getLogger()->info(__VA_ARGS__)
#define LOGW(...) maple::Console::getLogger()->warn(__VA_ARGS__)
#define LOGE(...) maple::Console::getLogger()->error(__VA_ARGS__)
#define LOGC(...) maple::Console::getLogger()->critical(__VA_ARGS__)
