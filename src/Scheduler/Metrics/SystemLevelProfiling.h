#ifndef _HARMONIC_SCHEDULER_PROFILING_METRICS_SYSTEM_LEVEL_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_METRICS_SYSTEM_LEVEL_PROFILING_h

#include "../AbstractScheduler.h"

namespace Harmonic
{
	namespace Profiling
	{
		namespace Metrics
		{
			/// <summary>
			/// SchedulerBaseProfiling: Scheduler loop with basic profiling timing statistics.
			/// Implements Profiling::IBaseProfiler for trace retrieval.
			/// 
			/// Collects coarse-grained timing statistics across all tasks:
			/// - Total busy time (sum of all task execution durations)
			/// - Total idle sleep time
			/// - Total scheduler overhead excluding task execution and idle sleep
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
			template<task_index_t MaxTaskCount, bool IdleSleepEnabled = false>
			class SystemLevelProfiling : public ISystemLevelProfiler, public AbstractScheduler<MaxTaskCount>
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
				SystemMetrics System{};

				ISystemLevelListener* ResultListener = nullptr;

			public:
				SystemLevelProfiling()
					: ISystemLevelProfiler()
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
				virtual bool RequestMetrics(ISystemLevelListener* resultListener) override
				{
					ResultListener = resultListener;

					return ResultListener != nullptr;
				}

				virtual void ResetMetrics() override
				{
					System = SystemMetrics{};
				}

				/// <summary>
				/// Main scheduler loop with basic profiling.
				/// 
				/// Executes one scheduler iteration:
				/// 1. Records loop start time
				/// 2. Checks each task and runs those that are due, measuring task execution time
				/// 3. Optionally enters idle sleep when IdleSleepEnabled is true and no task runs
				/// 4. Records scheduler overhead excluding task execution and sleep
				/// 5. Increments iteration counter
				/// 
				/// Profiling measurements:
				/// - SchedulerAggregate.Busy: Cumulative time spent executing tasks (microseconds)
				/// - SchedulerAggregate.Scheduling: Cumulative scheduler overhead excluding task execution and sleep (microseconds)
				/// - SchedulerAggregate.IdleSleep: Cumulative time spent in idle sleep (microseconds)
				/// - SchedulerAggregate.Iterations: Number of Loop() calls (scheduler tick count)
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
					uint32_t measure = 0; // Reusable timestamp for measuring individual segments.
					const uint32_t loopStart = Platform::GetProfilerTimestamp();

					if (ResultListener != nullptr && System.HasData())
					{
						NotifyProfileResult();
						measure = Platform::GetProfilerTimestamp();
					}
					else
					{
						measure = loopStart;
					}

					// Reset per-iteration activity before dispatch. Task execution and,
					// when enabled, registry mutations set Hot again during this iteration.
					Hot = false;

					// Run all tasks that are due, measuring approximate busy time (actual task execution).
					for (task_index_t i = 0; i < TaskCount; i++)
					{
						if (Tasks[i].RunIfTime())
						{
							// Task executed: accumulate its approximate duration.
							const uint32_t elapsed = Platform::GetProfilerTimestamp() - measure;
							System.Busy += elapsed; // Append task busy time.
							System.Scheduling -= elapsed; // Subtract task busy time from scheduling overhead.

							// Optimization: under heavy load, skip idle sleep checks.
							Hot = true;

							// Reset the measurement timestamp for the next task execution.
							measure += elapsed;
						}
					}

					// Optional idle sleep with timing.
					if (!Hot)
					{
						// No task or registry activity occurred: enter low-power sleep.
						measure = Platform::GetProfilerTimestamp();
						IdleSleep();
						const uint32_t elapsed = Platform::GetProfilerTimestamp() - measure;
						System.IdleSleep += elapsed; // Append idle sleep time.
						System.Scheduling -= elapsed; // Subtract idle sleep time from scheduling overhead.
					}

					// Record active scheduler overhead, excluding task execution and sleep.
					System.Scheduling += Platform::GetProfilerTimestamp() - loopStart;
				}

				void Loop(ConditionalDispatch::FalseType)
				{
					uint32_t measure = 0; // Reusable timestamp for measuring individual segments.
					const uint32_t loopStart = Platform::GetProfilerTimestamp();

					if (ResultListener != nullptr && System.HasData())
					{
						NotifyProfileResult();
						measure = Platform::GetProfilerTimestamp();
					}
					else
					{
						measure = loopStart;
					}

					// Run all tasks that are due, measuring busy time (actual task execution).
					for (task_index_t i = 0; i < TaskCount; i++)
					{
						if (Tasks[i].RunIfTime())
						{
							// Task executed: accumulate its approximate duration.
							const uint32_t elapsed = Platform::GetProfilerTimestamp() - measure;
							System.Busy += elapsed; // Append task busy time.
							System.Scheduling -= elapsed; // Subtract task busy time from scheduling overhead.

							// Reset the measurement timestamp for the next task execution or sleep check.
							measure += elapsed;
						}
					}

					// Record active scheduler overhead, excluding task execution.
					System.Scheduling += Platform::GetProfilerTimestamp() - loopStart;
				}

			private:
				void NotifyProfileResult()
				{
					// Store a temporary copy of the listener and clear the member to avoid reentrancy issues.
					// This allows the listener to immediately request another profile if desired.
					const auto listener = ResultListener;
					ResultListener = nullptr;

					listener->OnMetricsResult(System);

					// Clear the profiling data after notifying the listener to prepare for the next measurement window.
					ResetMetrics();
				}
			};
		}
	}

	// Alias for the system-level metrics profiling scheduler.
	template<task_index_t MaxTaskCount, bool IdleSleepEnabled>
	using SchedulerMetricsSystemLevel = Profiling::Metrics::SystemLevelProfiling<MaxTaskCount, IdleSleepEnabled>;
}
#endif