//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef MAPLE_PROFILE
#	ifdef PLATFORM_WINDOWS
#		define TRACY_CALLSTACK 1
#	endif
#	include <Tracy.hpp>
#	define PROFILE_SCOPE(name) ZoneScopedN(name)
#	define PROFILE_FUNCTION() ZoneScoped
#	define PROFILE_FRAMEMARKER() FrameMark
#	define PROFILE_LOCK(type, var, name) TracyLockableN(type, var, name)
#	define PROFILE_LOCKMARKER(var) LockMark(var)
#	define PROFILE_SETTHREADNAME(name) tracy::SetThreadName(name)

#else
#	define PROFILE_SCOPE(name)
#	define PROFILE_FUNCTION()
#	define PROFILE_FRAMEMARKER()
#	define PROFILE_LOCK(type, var, name) type var
#	define PROFILE_LOCKMARKER(var)
#	define PROFILE_SETTHREADNAME(name)
#endif

namespace maple::global::component
{
	struct Profiler
	{
		uint32_t frameCount;
		uint32_t drawCall;
		float fps;
	};
}