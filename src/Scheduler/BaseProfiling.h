#ifndef _HARMONIC_SCHEDULER_BASE_PROFILER_h
#define _HARMONIC_SCHEDULER_BASE_PROFILER_h

#include "Abstract.h"
#include "../Platform/ConditionalDispatch.h"

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
	/// Profiling data is accumulated until a trace is requested with RequestTrace().
	/// The result is delivered asynchronously to the supplied listener at the end of
	/// a scheduler loop iteration, after which the trace is cleared.
	/// 
	/// Usage:
	/// Call Loop() as frequently as possible (typically in main loop).
	/// To receive traces, periodically call RequestTrace() with an IBaseProfilerListener implementation.
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
	template<task_handle_t MaxTaskCount, bool IdleSleepEnabled = false>
	class SchedulerBaseProfiling : public Profiling::IBaseProfiler, public AbstractScheduler<MaxTaskCount>
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
		/// Accumulated profiling trace for the current measurement window.
		/// Reset to zero after the trace listener is notified.
		/// </summary>
		Profiling::BaseTrace Trace{};

		Profiling::IBaseProfilerListener* ResultListener = nullptr;

	public:
		SchedulerBaseProfiling()
			: Profiling::IBaseProfiler()
			, Base(IdleSleepEnabled)
		{}

		/// <summary>
		/// Requests the accumulated profiling trace asynchronously.
		/// The result is delivered to the supplied listener at the end of a
		/// scheduler loop iteration. After the listener is notified, the trace
		/// data is cleared and a new measurement window begins.
		/// </summary>
		/// <param name="resultListener">Listener that receives the trace result.</param>
		/// <returns>True if the listener was accepted; false if it is null.</returns>
		virtual bool RequestTrace(Profiling::IBaseProfilerListener* resultListener) override
		{
			ResultListener = resultListener;

			return ResultListener != nullptr;
		}

		virtual void ResetTrace() override
		{
			ClearTraceData();
		}

		/// <summary>
		/// Main scheduler loop with basic profiling.
		/// 
		/// Executes one scheduler iteration:
		/// 1. Records loop start time
		/// 2. Checks each task and runs those that are due, measuring task execution time
		/// 3. Optionally enters idle sleep when IdleSleepEnabled is true and no task runs
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
			Loop(IdleSleepTag{});
		}

	private:
		void Loop(ConditionalDispatch::TrueType)
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = 0; // Reusable timestamp for measuring individual segments.

			// Reset per-iteration activity before dispatch. Task execution and,
			// when enabled, registry mutations set Hot again during this iteration.
			Hot = false;

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

			// Optional idle sleep with timing.
			if (!Hot)
			{
				// No task or registry activity occurred: enter low-power sleep.
				IdleSleep();
				Trace.IdleSleep += Platform::GetProfilerTimestamp() - measure;
			}

			// Record total scheduling time (from loop start to now, excluding sleep).
			Trace.Iterations++;
			Trace.Scheduling += measure - loopStart;

			if (ResultListener != nullptr && Trace.Iterations > 0)
			{
				// Store a temporary copy of the listener and clear the member to avoid reentrancy issues.
				// This allows the listener to immediately request another trace if desired.
				const auto listener = ResultListener;
				ResultListener = nullptr;

				listener->OnTraceResult(Trace);

				// Clear the trace data after notifying the listener to prepare for the next measurement window.
				ClearTraceData();
			}
		}

		void Loop(ConditionalDispatch::FalseType)
		{
			const uint32_t loopStart = Platform::GetProfilerTimestamp();
			uint32_t measure = Platform::GetProfilerTimestamp(); // Reusable timestamp for measuring individual segments.

			// Run all tasks that are due, measuring busy time (actual task execution).
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				if (Tasks[i].RunIfTime())
				{
					// Task executed: accumulate its duration.
					Trace.Busy += Platform::GetProfilerTimestamp() - measure;
				}
				measure = Platform::GetProfilerTimestamp();
			}

			// Record total scheduling time (from loop start to now).
			Trace.Iterations++;
			Trace.Scheduling += measure - loopStart;
		}

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