//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "JobSystem.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"
#include <sstream>

namespace maple::JobSystem
{
	namespace internal
	{
		template <typename T, size_t capacity>
		class RingBuffer
		{
		  public:
			inline bool push(const T &item)
			{
				//PROFILE_FUNCTION();
				bool result = false;
				lock.lock();
				size_t next = (head + 1) % capacity;
				if (next != tail)
				{
					data[head] = item;
					head       = next;
					result     = true;
				}
				lock.unlock();
				return result;
			}
			inline bool pop(T &item)
			{
				//PROFILE_FUNCTION();
				bool result = false;
				lock.lock();
				if (tail != head)
				{
					item   = data[tail];
					tail   = (tail + 1) % capacity;
					result = true;
				}
				lock.unlock();
				return result;
			}

		  private:
			T          data[capacity];
			size_t     head = 0;
			size_t     tail = 0;
			std::mutex lock;
		};
	}        // namespace internal

	struct Job
	{
		Context *                            ctx;
		std::function<void(JobDispatchArgs)> task;
		uint32_t                             groupID;
		uint32_t                             groupJobOffset;
		uint32_t                             groupJobEnd;
		uint32_t                             sharedMemorySize;
	};

	uint32_t                                  numThreads = 0;
	internal::RingBuffer<Job, 256>            jobQueue;
	std::condition_variable                   wakeCondition;
	std::mutex                                wakeMutex;
	std::vector<std::shared_ptr<std::thread>> threads;

	inline auto work()
	{
		Job job;
		if (jobQueue.pop(job))
		{
			JobDispatchArgs args;
			args.groupID = job.groupID;
			if (job.sharedMemorySize > 0)
			{
				args.sharedMemory = alloca(job.sharedMemorySize);        //a stack memory. so no need to delete.
			}
			else
			{
				args.sharedMemory = nullptr;
			}

			for (uint32_t i = job.groupJobOffset; i < job.groupJobEnd; ++i)
			{
				args.jobIndex          = i;
				args.groupIndex        = i - job.groupJobOffset;
				args.isFirstJobInGroup = (i == job.groupJobOffset);
				args.isLastJobInGroup  = (i == job.groupJobEnd - 1);
				job.task(args);
			}

			if (job.ctx)
				job.ctx->counter.fetch_sub(1);
			return true;
		}
		return false;
	}

	auto init(uint32_t maxCores) -> void
	{
		PROFILE_FUNCTION();
		const auto numCores = std::thread::hardware_concurrency();
		numThreads          = std::max(1u, numCores - 1);
		numThreads = std::min(maxCores, numThreads);
		for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread worker([threadID] {
                            std::stringstream ss;
                            ss << "WorkerThread_" << threadID;
                            PROFILE_SETTHREADNAME(ss.str().c_str());

							while(true)
                            {
                                if(!work())
                                {
                                    // no job, put thread to sleep
                                    std::unique_lock<std::mutex> lock(wakeMutex);
                                    wakeCondition.wait(lock);
                                }
                            } });

			HANDLE handle = (HANDLE) worker.native_handle();

			DWORD_PTR affinityMask   = 1ull << threadID;
			DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
			MAPLE_ASSERT(affinityResult > 0, "");

			BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			MAPLE_ASSERT(priorityResult != 0, "");

			std::wstringstream wss;
			wss << "WorkerThread_" << threadID;
			HRESULT hr = SetThreadDescription(handle, wss.str().c_str());

			MAPLE_ASSERT(SUCCEEDED(hr), "");

			worker.detach();
		}

		LOGI("Initialized JobSystem with [{0} cores] [{1} threads]", numCores, numThreads);
	}

	auto getThreadCount() -> uint32_t
	{
		return numThreads;
	}

	auto executeInternal(Context *ctx, const std::function<void(JobDispatchArgs)> &task) -> void
	{
		if (ctx)
			ctx->counter.fetch_add(1);

		Job job;
		job.ctx              = ctx;
		job.task             = task;
		job.groupID          = 0;
		job.groupJobOffset   = 0;
		job.groupJobEnd      = 1;
		job.sharedMemorySize = 0;

		while (!jobQueue.push(job))        // push fail, so consume it in context.
		{
			wakeCondition.notify_all();
			work();
		}

		wakeCondition.notify_one();
	}

	auto dispatchInternal(Context *ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &task, size_t sharedMemorySize) -> void
	{
		PROFILE_FUNCTION();
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}

		const uint32_t groupCount = dispatchGroupCount(jobCount, groupSize);

		if (ctx)
			ctx->counter.fetch_add(groupCount);

		Job job;
		job.ctx              = ctx;
		job.task             = task;
		job.sharedMemorySize = (uint32_t) sharedMemorySize;

		for (uint32_t groupID = 0; groupID < groupCount; ++groupID)
		{
			job.groupID        = groupID;
			job.groupJobOffset = groupID * groupSize;
			job.groupJobEnd    = std::min(job.groupJobOffset + groupSize, jobCount);

			while (!jobQueue.push(job))
			{
				wakeCondition.notify_all();
				work();
			}
		}
	}

	auto execute(Context &ctx, const std::function<void(JobDispatchArgs)> &task) -> void
	{
		executeInternal(&ctx, task);
	}

	auto execute(const std::function<void(JobDispatchArgs)> &task) -> void
	{
		executeInternal(nullptr, task);
	}

	auto dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &task, size_t sharedMemorySize /*= 0*/) -> void
	{
		dispatchInternal(nullptr, jobCount, groupSize, task, sharedMemorySize);
	}

	auto dispatch(Context &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)> &task, size_t sharedMemorySize /*= 0*/) -> void
	{
		dispatchInternal(&ctx, jobCount, groupSize, task, sharedMemorySize);
	}

	auto dispatchGroupCount(uint32_t jobCount, uint32_t groupSize) -> uint32_t
	{
		return (jobCount + groupSize - 1) / groupSize;
	}

	auto isBusy(const Context &ctx) -> bool
	{
		return ctx.counter.load() > 0;
	}

	auto wait(const Context &ctx) -> void
	{
		wakeCondition.notify_all();
		while (isBusy(ctx))
		{
			work();
		}
	}
}        // namespace maple::JobSystem