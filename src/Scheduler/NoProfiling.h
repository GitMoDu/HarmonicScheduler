#ifndef _HARMONIC_SCHEDULER_NO_PROFILER_h
#define _HARMONIC_SCHEDULER_NO_PROFILER_h

#include "Abstract.h"

namespace Harmonic
{
	/// <summary>
	/// SchedulerNoProfiling provides a lightweight cooperative task scheduler with no profiling overhead.
	/// 
	/// This is the most efficient scheduler variant, optimized for:
	/// - Minimal memory footprint (no profiling buffers)
	/// - Lowest per-loop overhead (no timestamp reads)
	/// - Production deployments where profiling is not needed
	/// 
	/// Features:
	/// - Dynamic task registration via inherited TaskRegistry interface
	/// - Optional low-power idle sleep (compile-time configurable)
	/// - Zero profiling overhead (no timestamps, counters, or trace data)
	/// 
	/// Trade-offs vs profiled schedulers:
	/// - No visibility into CPU usage, task execution time, or performance metrics
	/// - Faster loop execution
	/// - Lower memory usage (no trace buffers)
	/// 
	/// When to use:
	/// - Production builds after profiling/optimization is complete
	/// - Tight memory constraints (8-bit MCUs, small RAM)
	/// - Maximum performance requirements
	/// 
	/// When NOT to use:
	/// - During development/debugging (use BaseProfilerScheduler or FullProfilerScheduler)
	/// - When diagnosing performance issues
	/// - When monitoring CPU usage or task timing
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_id_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerNoProfiling : public AbstractScheduler<MaxTaskCount>
	{
	private:
		using Base = AbstractScheduler<MaxTaskCount>;
		static_assert(MaxTaskCount <= TASK_MAX_COUNT, "MaxTaskCount exceeds platform maximum task count (TASK_MAX_COUNT)");

	protected:
		using Base::Tasks;
		using Base::TaskCount;
		using Base::Hot;
		using Base::IdleSleep;

	public:
		SchedulerNoProfiling() : Base(IdleSleepEnabled) {}

		/// <summary>
		/// Main scheduler loop without profiling.
		/// 
		/// Executes one scheduler iteration with minimal overhead:
		/// 1. Checks each task and runs those that are due
		/// 2. Optionally enters idle sleep if no tasks ran (when IdleSleepEnabled is true)
		/// 
		/// Performance characteristics:
		/// - No timestamp reads (zero micros() overhead)
		/// - No trace data accumulation (zero memory writes)
		/// - Direct task dispatch (minimal branching)
		/// 
		/// Idle sleep behavior (when IdleSleepEnabled is true):
		/// - Hot flag tracks whether any task executed in this iteration, or if registry changed
		/// - If Hot flag is false, enters low-power idle sleep
		/// - Sleep duration is determined by platform-specific IdleSleep() implementation
		/// - Scheduler wake sources: next task deadline or interrupt (e.g., WakeFromISR)
		/// 
		/// Idle sleep optimization (when IdleSleepEnabled is false):
		/// - Hot flag tracking is usually compiled out (zero overhead)
		/// - No idle sleep checks (minimal branching)
		/// - Tightest possible scheduling loop
		/// 
		/// Should be called as frequently as possible (typically in main loop).
		/// </summary>
		void Loop()
		{
			// Compile-time switch for idle sleep feature.
			// Optimizer will eliminate the unused branch entirely.
			if (IdleSleepEnabled)
			{
				// Reset hot flag before checking tasks.
				Hot = false;

				// Run all tasks that are due.
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].RunIfTime())
					{
						// Optimization: under heavy load, skip idle sleep checks.
						Hot = true;
					}
				}

				// Enter idle sleep only if no tasks ran and registry is stable.
				// This reduces power consumption during idle periods.
				if (!Hot)
				{
					IdleSleep();
				}
			}
			else
			{
				// Idle sleep disabled: run tasks without hot flag tracking.
				// This is the tightest possible scheduling loop.
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					Tasks[i].RunIfTime();
				}
			}
		}
	};
}
#endif