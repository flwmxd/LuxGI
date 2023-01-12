//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <stdint.h>

namespace maple
{
	namespace trace
	{
		enum Id : int32_t
		{
			SoftTrace,
			RayTrace,
			Length,
		};

		static const char *Names[] =
		    {
		        "Soft Trace",
		        "Ray Trace",
		        nullptr};

		namespace global::component
		{
			struct RaytraceConfig
			{
				trace::Id traceType = trace::SoftTrace;
			};
		}

		inline auto isSoftTrace(const global::component::RaytraceConfig &config)
		{
			return config.traceType == trace::SoftTrace;
		}
	}        // namespace trace
};        // namespace maple