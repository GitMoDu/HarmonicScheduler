#ifndef _HARMONIC_SCHEDULER_NO_PROFILER_h
#define _HARMONIC_SCHEDULER_NO_PROFILER_h

#include "AbstractScheduler.h"

namespace Harmonic
{
	/// <summary>
	/// SchedulerNoProfiling provides a lightweight cooperative task scheduler with no profiling overhead.
	/// 
	/// This is the most efficient scheduler variant, optimized for:
	/// - Minimal memory footprint (no profiling buffers)
	/// - Lowest scheduler-side overhead beyond the required millisecond timebase checks
	/// - Production deployments where profiling is not needed
	/// 
	/// Features:
	/// - Dynamic task registration via inherited TaskRegistry interface
	/// - Optional low-power idle sleep (compile-time configurable)
	/// - Zero profiling overhead (no microsecond profiler timestamps, counters, or trace data)
	/// 
	/// Trade-offs vs profiled schedulers:
	/// - No visibility into CPU usage, task execution time, or performance metrics
	/// - Faster loop execution
	/// - Lower memory usage (no trace buffers)
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_index_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerNoProfiling : public AbstractScheduler<MaxTaskCount>
	{
	private:
		using Base = AbstractScheduler<MaxTaskCount>;
		using IdleSleepTag = typename ConditionalDispatch::conditional_type<IdleSleepEnabled>::type;

		static_assert(MaxTaskCount <= TASK_MAX_COUNT, "MaxTaskCount exceeds platform maximum task count (TASK_MAX_COUNT)");

	protected:
		using Base::Tasks;
		using Base::TaskCount;
		using Base::Hot;
		using Base::IdleSleep;

	public:
		SchedulerNoProfiling()
			: Base(IdleSleepEnabled)
		{}

		/// <summary>
		/// Main scheduler loop without profiling.
		/// 
		/// Executes one scheduler iteration with minimal overhead:
		/// 1. Checks each task and runs those that are due
		/// 2. Optionally enters idle sleep if no tasks ran (when IdleSleepEnabled is true)
		/// 
		/// Performance characteristics:
		/// - No additional profiler timestamp reads (zero micros() overhead on Arduino)
		/// - No trace data accumulation; task timing state is still updated by RunIfTime()
		/// - Direct task dispatch (minimal branching)
		/// 
		/// Idle sleep behavior (when IdleSleepEnabled is true):
		/// - Hot flag tracks whether any task executed in this iteration, or if registry changed
		/// - If Hot flag is false, enters low-power idle sleep
		/// - Sleep duration is determined by platform-specific IdleSleep() implementation
		/// - Scheduler wake sources: next task deadline or interrupt (e.g., WakeFromISR)
		/// 
		/// Idle sleep optimization (when IdleSleepEnabled is false):
		/// - Hot flag tracking is not used
		/// - No idle sleep checks
		/// - Tightest possible scheduling loop
		/// 
		/// Should be called as frequently as possible (typically in main loop).
		/// </summary>
		void Loop()
		{
			Loop(IdleSleepTag{});
		}

	private:
		void Loop(ConditionalDispatch::TrueType)
		{
			// Reset per-iteration activity before dispatch. RunIfTime() and, when
			// enabled, registry mutations can set Hot again during this iteration.
			Hot = false;

			// Run all tasks that are due.
			for (task_index_t i = 0; i < TaskCount; i++)
			{
				if (Tasks[i].RunIfTime())
				{
					// Optimization: under heavy load, skip idle sleep checks.
					Hot = true;
				}
			}

			// Enter idle sleep only when neither task execution nor registry
			// activity made this iteration hot. Hot is ignored when idle sleep is
			// disabled because the alternate loop does not inspect it.
			if (!Hot)
			{
				IdleSleep();
			}
		}

		void Loop(ConditionalDispatch::FalseType)
		{
			// Idle sleep disabled: run tasks without hot flag tracking.
			// This is the tightest possible scheduling loop.
			for (task_index_t i = 0; i < TaskCount; i++)
			{
				Tasks[i].RunIfTime();
			}
		}
	};
}
#endif