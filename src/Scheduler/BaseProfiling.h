#ifndef _HARMONIC_SCHEDULER_BASE_PROFILER_h
#define _HARMONIC_SCHEDULER_BASE_PROFILER_h

#include "Abstract.h"

namespace Harmonic
{
	/// <summary>
	/// SchedulerBaseProfiling: Scheduler loop with basic profiling timing statistics.
	/// Implements Profiling::IBaseProfiler for trace retrieval.
	/// 
	/// Collects coarse-grained timing statistics across all tasks:
	/// - Total busy time (sum of all task execution durations)
	/// - Total idle sleep time
	/// - Total idle + scheduling overhead
	/// - Loop iteration count
	/// 
	/// Does NOT track per-task statistics. For per-task profiling, use FullProfilerScheduler.
	/// 
	/// Profiling data is accumulated until retrieved via GetTrace(), which atomically
	/// copies and clears the trace. This design prevents data races and ensures each
	/// trace represents a discrete time window.
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// For traces, periodically call GetTrace() to retrieve and reset profiling data.
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_id_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerBaseProfiling : public Profiling::IBaseProfiler, public AbstractScheduler<MaxTaskCount>
	{
	private:
		using Base = AbstractScheduler<MaxTaskCount>;
		static_assert(MaxTaskCount <= TASK_MAX_COUNT, "MaxTaskCount exceeds platform maximum task count (TASK_MAX_COUNT)");

	protected:
		using Base::Tasks;
		using Base::TaskCount;
		using Base::Hot;
		using Base::IdleSleep;

	private:
		/// <summary>
		/// Accumulated profiling trace for the current measurement window.
		/// Reset to zero after each GetTrace() call.
		/// </summary>
		Profiling::BaseTrace Trace{};

	public:
		SchedulerBaseProfiling()
			: Profiling::IBaseProfiler()
			, Base(IdleSleepEnabled)
		{
		}

		/// <summary>
		/// Retrieves and clears the accumulated profiling trace.
		/// 
		/// This method atomically copies the current trace data and resets all counters.
		/// Each trace represents the time period since the last GetTrace() call (or since
		/// scheduler start if this is the first call).
		/// 
		/// Typical usage:
		///   - Call this periodically (e.g., every 1 second via a logging task)
		///   - Analyze the returned trace to calculate CPU usage, idle time, etc.
		/// 
		/// Returns false if no iterations have occurred since the last call, indicating
		/// no useful data is available.
		/// </summary>
		/// <param name="trace">Output structure that receives the aggregated profiling data.</param>
		/// <returns>True if trace contains valid data (at least one iteration); false otherwise.</returns>
		bool GetTrace(Profiling::BaseTrace& trace) override
		{
			if (Trace.Iterations == 0)
			{
				return false; // No trace data available.
			}

			// Copy overall trace.
			trace = Trace;

			// Clear trace data after retrieval.
			ClearTraceData();

			return true;
		}

		/// <summary>
		/// Main scheduler loop with basic profiling.
		/// 
		/// Executes one scheduler iteration:
		/// 1. Records loop start time
		/// 2. Checks each task and runs those that are due, measuring task execution time
		/// 3. Optionally enters idle sleep if (when IdleSleepEnabled is true)
		/// 4. Records total idle + scheduling overhead (includes task dispatch time but excludes sleep)
		/// 5. Increments iteration counter
		/// 
		/// Profiling measurements:
		/// - Trace.Busy: Cumulative time spent executing tasks (microseconds)
		/// - Trace.Scheduling: Cumulative time for idle + scheduler overhead + task execution (microseconds)
		/// - Trace.IdleSleep: Cumulative time spent in idle sleep (microseconds)
		/// - Trace.Iterations: Number of Loop() calls (scheduler tick count)
		/// 
		/// Should be called as frequently as possible (typically in main loop).
		/// </summary>
		void Loop()
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = 0; // Reusable timestamp for measuring individual segments.

			// Compile-time check for idle sleep feature, optimized out if disabled.
			if (IdleSleepEnabled)
			{
				// Reset hot flag before looping all tasks.
				// If any task runs or registry changes, Hot will be set to true.
				Hot = false;
			}

			// Run all tasks that are due, measuring busy time (actual task execution).
			measure = Platform::GetProfilerTimestamp();
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				if (Tasks[i].RunIfTime())
				{
					// Task executed: accumulate its duration.
					Trace.Busy += Platform::GetProfilerTimestamp() - measure;

					// Optimization: under heavy load, skip idle sleep checks.
					Hot = true;
				}
				measure = Platform::GetProfilerTimestamp();
			}

			// Optional idle sleep with timing, optimized out if disabled.
			if (IdleSleepEnabled && !Hot)
			{
				// No tasks ran and registry is stable: enter low-power sleep.
				IdleSleep();
				Trace.IdleSleep += Platform::GetProfilerTimestamp() - measure;
			}

			// Record total scheduling time (from loop start to now, excluding sleep).
			// This includes task dispatch overhead, task execution time, and any other
			// bookkeeping. Sleep time is tracked separately.
			Trace.Iterations++;
			Trace.Scheduling += measure - loopStart;
		}

	private:
		void ClearTraceData()
		{
			Trace.Iterations = 0;
			Trace.Scheduling = 0;
			Trace.Busy = 0;
			Trace.IdleSleep = 0;
		}
	};
}
#endif