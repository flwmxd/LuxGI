//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace maple
{
	class Timestep
	{
	  public:
		Timestep(float initTime = 0.0f) :
		    time(initTime)
		{
		}

		operator float() const
		{
			return time;
		}

		inline auto getSeconds() const
		{
			return time;
		}
		inline auto getMilliseconds() const
		{
			return time * 1000.0f;
		}

	  private:
		float time;
	};
};        // namespace maple