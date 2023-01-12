//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <chrono>
#include <cstdint>

namespace maple
{
	// Small class for measuring elapsed time between game loops.
	class Timer
	{
	  public:
		Timer()
		{
			start();
		}

		~Timer()
		{
		}

		// Start the timer by setting the last measurement to now.
		inline auto start() -> void
		{
			point = clock::now();
			prev  = current();
		}

		// Return time elapsed since the last measurement.
		inline auto stop() -> int64_t
		{
			clock::time_point last = point;
			point                  = clock::now();
			auto duration          = std::chrono::duration_cast<std::chrono::microseconds>(point - last);
			return duration.count();
		}

		inline auto currentTimestamp()
		{
			auto tp  = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
			return tmp.count();
		}

		inline auto current() -> std::chrono::high_resolution_clock::time_point
		{
			return std::chrono::high_resolution_clock::now();
		}

		inline auto elapsed(std::chrono::high_resolution_clock::time_point begin,
		                    std::chrono::high_resolution_clock::time_point end)
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		}

		inline auto step() -> float
		{
			auto currTime = current();

			auto ela = elapsed(prev, currTime);

			prev = currTime;

			return ela / 1000000.f;
		}

	  private:
		using clock = std::chrono::high_resolution_clock;
		clock::time_point point;

		std::chrono::high_resolution_clock::time_point prev;
	};

};        // namespace maple
