#ifndef _HARMONIC_SCHEDULER_FULL_PROFILER_h
#define _HARMONIC_SCHEDULER_FULL_PROFILER_h

#include "Abstract.h"

namespace Harmonic
{
	/// <summary>
	/// SchedulerFullProfiling: Scheduler loop with full per-task profiling and timing statistics.
	/// Implements Profiling::IFullProfiler for trace retrieval.
	/// 
	/// Collects detailed timing statistics for each individual task plus global metrics:
	/// - Per-task execution time (cumulative duration)
	/// - Per-task maximum execution time (worst-case spike)
	/// - Per-task iteration count (how many times each task ran)
	/// - Total idle sleep time
	/// - Total idle time
	/// - Loop iteration count
	/// - Total trace time
	/// 
	/// Use cases:
	/// - Identifying which specific tasks consume the most CPU
	/// - Detecting timing anomalies (via max duration tracking)
	/// - Optimizing task distribution and scheduling
	/// - Profiling real-time performance characteristics
	/// 
	/// Trade-offs vs BaseProfilerScheduler:
	/// - Higher memory cost: O(MaxTaskCount) vs O(1)
	/// - Higher base per-loop overhead, increases with task count
	/// - Task level granularity vs aggregate only
	/// 
	/// Handles dynamic task count changes gracefully by detecting mismatches
	/// and resetting trace data to prevent stale/inconsistent statistics.
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// For traces, periodically call GetTrace() to retrieve and reset profiling data.
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_id_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerFullProfiling : public Profiling::IFullProfiler, public AbstractScheduler<MaxTaskCount>
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
		/// Per-task profiling data array, indexed by task ID.
		/// Stores cumulative duration, max duration, and iteration count for each task.
		/// Reset to zero after each GetTrace() call.
		/// </summary>
		Profiling::TaskTrace TaskTraces[MaxTaskCount]{};

		/// <summary>
		/// Global profiling trace for the current measurement window.
		/// Includes total scheduling overhead, idle sleep time, iteration count, and task count.
		/// Reset to zero after each GetTrace() call.
		/// </summary>
		Profiling::FullTrace Trace{};

	public:
		SchedulerFullProfiling()
			: Profiling::IFullProfiler()
			, Base(IdleSleepEnabled)
		{
		}

		/// <summary>
		/// Retrieves and clears accumulated profiling data for all tasks and global metrics.
		/// 
		/// This method atomically copies the current trace data (both global and per-task)
		/// and resets all counters. Each trace represents the time period since the last
		/// GetTrace() call (or since scheduler start if this is the first call).
		/// 
		/// The caller must provide a buffer large enough to hold per-task traces. If the
		/// buffer is smaller than the actual task count, only the first maxTraces tasks
		/// will be copied (safe truncation).
		/// 
		/// Typical usage:
		///   - Call this periodically (e.g., every 1 second via a logging task)
		///   - Analyze global trace (CPU usage, idle time, etc.)
		///   - Iterate through per-task traces to identify hotspots
		/// 
		/// Returns false if no iterations have occurred since the last call, indicating
		/// no useful data is available.
		/// </summary>
		/// <param name="trace">Output structure that receives global profiling data (scheduling, idle, iterations).</param>
		/// <param name="tracesBuffer">Output buffer to receive per-task profiling data (must be at least maxTraces elements).</param>
		/// <param name="maxTraces">Size of the tracesBuffer array (maximum number of task traces to copy).</param>
		/// <returns>True if trace contains valid data (at least one iteration); false otherwise.</returns>
		bool GetTrace(Profiling::FullTrace& trace, Profiling::TaskTrace* tracesBuffer, const uint8_t maxTraces) override
		{
			if (Trace.Iterations == 0)
			{
				return false; // No trace data available.
			}

			// Copy overall trace.
			trace = Trace;

			// Copy per-task traces up to the provided buffer size.
			const uint8_t traceCount = (Trace.TaskCount < maxTraces) ? Trace.TaskCount : maxTraces;
			for (uint_fast8_t i = 0; i < traceCount; i++)
			{
				tracesBuffer[i] = this->TaskTraces[i];
			}

			ClearTraceData();

			return true;
		}

		/// <summary>
		/// Resets all profiling counters (global and per-task) to zero.
		/// Called automatically by GetTrace() after copying data.
		/// Also called automatically when task count changes to prevent stale data.
		/// Can be called manually to discard accumulated data and start a fresh measurement window.
		/// </summary>
		void ClearTraceData()
		{
			// Clear global trace.
			Trace.Iterations = 0;
			Trace.IdleSleep = 0;
			Trace.Scheduling = 0;

			// Clear per-task traces.
			for (uint_fast8_t i = 0; i < MaxTaskCount; i++)
			{
				TaskTraces[i].Duration = 0;
				TaskTraces[i].MaxDuration = 0;
				TaskTraces[i].Iterations = 0;
			}
		}

		/// <summary>
		/// Main scheduler loop with full per-task profiling.
		/// 
		/// Executes one scheduler iteration:
		/// 1. Records loop start time
		/// 2. Detects task count changes and resets trace if necessary (prevents stale data)
		/// 3. Checks each task and runs those that are due, measuring individual execution time
		/// 4. Tracks per-task statistics: cumulative duration, max duration, iteration count
		/// 5. Optionally enters idle sleep if no tasks ran (when IdleSleepEnabled is true)
		/// 6. Records total scheduling overhead (task dispatch + execution time)
		/// 7. Increments global iteration counter
		/// 
		/// Profiling measurements (global):
		/// - Trace.Scheduling: Cumulative time for scheduler overhead + all task execution (microseconds)
		/// - Trace.IdleSleep: Cumulative time spent in idle sleep (microseconds)
		/// - Trace.Iterations: Number of Loop() calls (scheduler tick count)
		/// - Trace.TaskCount: Number of active tasks (snapshot at trace window start)
		/// 
		/// Profiling measurements (per-task):
		/// - TaskTraces[i].Duration: Cumulative execution time for task i (microseconds)
		/// - TaskTraces[i].MaxDuration: Worst-case execution time for task i (microseconds)
		/// - TaskTraces[i].Iterations: Number of times task i executed
		/// 
		/// Note: Sum of TaskTraces[].Duration equals total busy time (task execution).
		///       Trace.Scheduling includes all task execution plus scheduler dispatch overhead.
		/// 
		/// Task count change handling:
		/// If TaskCount changes mid-trace (task attached/detached), all profiling data is
		/// cleared to prevent mixing statistics from different task configurations.
		/// 
		/// Should be called as frequently as possible (typically in main loop).
		/// </summary>
		void Loop()
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = 0; // Reusable timestamp for measuring individual task segments.

			// Initialize TaskCount on first iteration, or detect changes mid-trace.
			if (Trace.Iterations == 0)
			{
				// First iteration: snapshot the current task count.
				Trace.TaskCount = TaskCount;
			}
			else if (Trace.TaskCount != TaskCount)
			{
				// Task count changed (attach/detach): clear stale data and resync.
				ClearTraceData();
				Trace.TaskCount = TaskCount;
			}

			// Compile-time check for idle sleep feature, optimized out if disabled.
			if (IdleSleepEnabled)
			{
				// Reset hot flag before looping all tasks.
				// If any task runs or registry changes, Hot will be set to true.
				Hot = false;
			}

			// Run all tasks that are due, measuring each task's execution time individually.
			for (uint_fast8_t i = 0; i < Trace.TaskCount; i++)
			{
				measure = Platform::GetProfilerTimestamp();
				if (Tasks[i].RunIfTime())
				{
					// Task executed: measure its duration and update statistics.
					measure = Platform::GetProfilerTimestamp() - measure;

					// Optimization: under heavy load, skip idle sleep checks.
					Hot = true;

					TaskTraces[i].Iterations++;
					TaskTraces[i].Duration += measure;

					// Track worst-case execution time for this task.
					if (TaskTraces[i].MaxDuration < measure)
					{
						TaskTraces[i].MaxDuration = measure;
					}
				}
			}

			// Timestamp after all tasks have run (before potential sleep).
			measure = Platform::GetProfilerTimestamp();

			// Optional idle sleep with timing, optimized out if disabled.
			if (IdleSleepEnabled && !Hot)
			{
				// No tasks ran and registry is stable: enter low-power sleep.
				IdleSleep();
				Trace.IdleSleep += Platform::GetProfilerTimestamp() - measure;
			}

			// Record total scheduling time (from loop start to end of task dispatch).
			// This includes task dispatch overhead and all task execution time.
			// Sleep time is tracked separately in Trace.IdleSleep.
			// 
			// To calculate pure scheduler overhead (dispatch only, not task execution):
			//   overhead = Trace.Scheduling - sum(TaskTraces[].Duration)
			Trace.Iterations++;
			Trace.Scheduling += measure - loopStart;
		}
	};
}
#endif