//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <stdint.h>

namespace maple
{
	namespace RaytraceScale
	{
		enum Id : int32_t
		{
			Full,
			Half,
			Quarter,
			Length
		};

		static const char *Names[] =
		    {
		        "Full",
		        "Half",
		        "Quarter",
		        nullptr};
	}        // namespace RaytraceScale
};           // namespace maple