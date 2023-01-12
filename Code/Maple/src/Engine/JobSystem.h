//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include "Engine/Core.h"

namespace maple::JobSystem
{
	struct JobDispatchArgs
	{
		uint32_t jobIndex;
		uint32_t groupID;
		uint32_t groupIndex;
		bool     isFirstJobInGroup;
		bool     isLastJobInGroup;
		void *   sharedMemory;
	};

	struct Context
	{
		std::atomic<uint32_t> counter{0};
	};

	auto init(uint32_t maxCores) -> void;

	auto MAPLE_EXPORT getThreadCount() -> uint32_t;
	auto MAPLE_EXPORT execute(const std::function<void(JobDispatchArgs)> &task) -> void;
	auto MAPLE_EXPORT execute(Context &ctx, const std::function<void(JobDispatchArgs)> &task) -> void;
	auto MAPLE_EXPORT dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &task, size_t sharedMemorySize = 0) -> void;
	auto MAPLE_EXPORT dispatch(Context &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &task, size_t sharedMemorySize = 0) -> void;
	auto MAPLE_EXPORT dispatchGroupCount(uint32_t jobCount, uint32_t groupSize) -> uint32_t;
	auto MAPLE_EXPORT isBusy(const Context &ctx) -> bool;
	auto MAPLE_EXPORT wait(const Context &ctx) -> void;
}// namespace maple::job_system