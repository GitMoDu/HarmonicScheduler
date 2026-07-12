#ifndef _HARMONIC_SCHEDULER_FULL_PROFILER_h
#define _HARMONIC_SCHEDULER_FULL_PROFILER_h

#include "Abstract.h"
#include "../Platform/ConditionalDispatch.h"

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
	/// Profiling data is accumulated until a trace is requested with RequestTrace().
	/// The result is delivered asynchronously to the supplied listener at the end of
	/// a scheduler loop iteration, after which the trace is cleared.
	/// 
	/// Use cases:
	/// - Identifying which specific tasks consume the most CPU or are overrunning their expected time budget
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
	/// and resetting trace data to prevent stale or inconsistent statistics.
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// To receive traces, periodically call RequestTrace() with an
	/// IFullProfilerListener implementation.
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_handle_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerFullProfiling : public Profiling::IFullProfiler, public AbstractScheduler<MaxTaskCount>
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

	private:
		/// <summary>
		/// Per-task profiling data array, indexed by task ID.
		/// Stores cumulative duration, max duration, and iteration count for each task.
		/// Reset to zero after the trace listener is notified.
		/// </summary>
		Profiling::TaskTrace TaskTraces[MaxTaskCount]{};

		/// <summary>
		/// Global profiling trace for the current measurement window.
		/// Includes total scheduling overhead, idle sleep time, iteration count, and task count.
		/// Reset to zero after the trace listener is notified.
		/// </summary>
		Profiling::FullTrace Trace{};
		Profiling::IFullProfilerListener* ResultListener = nullptr;

	public:
		SchedulerFullProfiling()
			: Profiling::IFullProfiler()
			, Base(IdleSleepEnabled)
		{}

		/// <summary>
		/// Requests accumulated profiling data for all tasks and global metrics
		/// asynchronously. The result is delivered to the supplied listener at the
		/// end of a scheduler loop iteration, after which the profiling data is
		/// cleared and a new measurement window begins.
		/// </summary>
		/// <param name="resultListener">Listener that receives the global trace and per-task trace array.</param>
		bool RequestTrace(Profiling::IFullProfilerListener* resultListener) override
		{
			ResultListener = resultListener;
			return ResultListener != nullptr;
		}

		void ResetTrace() override
		{
			ClearTraceData();
		}

		/// <summary>
		/// Resets all profiling counters (global and per-task) to zero.
		/// Called automatically after the trace listener is notified.
		/// Also called automatically when task count changes to prevent stale data.
		/// Can be called manually to discard accumulated data and start a fresh measurement window.
		/// </summary>
		void ClearTraceData()
		{
			Trace.Iterations = 0;
			Trace.IdleSleep = 0;
			Trace.Scheduling = 0;

			for (uint_fast8_t i = 0; i < MaxTaskCount; i++)
			{
				TaskTraces[i].Duration = 0;
				TaskTraces[i].MaxDuration = 0;
				TaskTraces[i].Iterations = 0;
				TaskTraces[i].Handle = (i < TaskCount)
					? Tasks[i].Handle
					: TASK_INVALID_HANDLE;
			}
		}

		void OnTaskCollectionChanged() override
		{
			ClearTraceData();
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
			Loop(IdleSleepTag{});
		}

	private:
		void Loop(ConditionalDispatch::TrueType)
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = 0; // Reusable timestamp for measuring individual task segments.

			if (Trace.Iterations == 0)
			{
				Trace.TaskCount = TaskCount;
			}
			else if (Trace.TaskCount != TaskCount)
			{
				ClearTraceData();
				Trace.TaskCount = TaskCount;
			}

			// Reset per-iteration activity before dispatch. Task execution and,
			// when enabled, registry mutations set Hot again during this iteration.
			Hot = false;

			// Run all tasks that are due, measuring each task's execution time individually.
			for (uint_fast8_t i = 0; i < Trace.TaskCount; i++)
			{
				measure = Platform::GetProfilerTimestamp();
				if (Tasks[i].RunIfTime())
				{
					measure = Platform::GetProfilerTimestamp() - measure;

					// Optimization: under running load, skip idle sleep checks.
					Hot = true;

					TaskTraces[i].Iterations++;
					TaskTraces[i].Duration += measure;

					if (measure > TaskTraces[i].MaxDuration)
					{
						TaskTraces[i].MaxDuration = measure;
					}
				}
			}

			measure = Platform::GetProfilerTimestamp();

			// Optional idle sleep with timing.
			if (!Hot)
			{
				// No task or registry activity occurred: enter low-power sleep.
				IdleSleep();
				Trace.IdleSleep += Platform::GetProfilerTimestamp() - measure;
			}

			Trace.Iterations++;
			Trace.Scheduling += measure - loopStart;
			NotifyTraceResult();
		}

		void Loop(ConditionalDispatch::FalseType)
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = 0; // Reusable timestamp for measuring individual task segments.

			if (Trace.Iterations == 0)
			{
				Trace.TaskCount = TaskCount;
			}
			else if (Trace.TaskCount != TaskCount)
			{
				ClearTraceData();
				Trace.TaskCount = TaskCount;
			}

			// Run all tasks that are due, measuring each task's execution time individually.
			for (uint_fast8_t i = 0; i < Trace.TaskCount; i++)
			{
				measure = Platform::GetProfilerTimestamp();
				if (Tasks[i].RunIfTime())
				{
					measure = Platform::GetProfilerTimestamp() - measure;

					TaskTraces[i].Iterations++;
					TaskTraces[i].Duration += measure;

					if (measure > TaskTraces[i].MaxDuration)
					{
						TaskTraces[i].MaxDuration = measure;
					}
				}
			}

			Trace.Iterations++;
			Trace.Scheduling += Platform::GetProfilerTimestamp() - loopStart;
			NotifyTraceResult();
		}

		void NotifyTraceResult()
		{
			if (ResultListener == nullptr || Trace.Iterations == 0)
			{
				return;
			}

			Profiling::IFullProfilerListener* listener = ResultListener;
			ResultListener = nullptr;
			listener->OnTraceResult(Trace, TaskTraces, Trace.TaskCount);
			ClearTraceData();
		}

	};
}
#endif