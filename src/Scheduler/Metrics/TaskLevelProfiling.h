#ifndef _HARMONIC_SCHEDULER_PROFILING_METRICS_TASK_LEVEL_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_METRICS_TASK_LEVEL_PROFILING_h

#include "../AbstractScheduler.h"

namespace Harmonic
{
	namespace Profiling
	{
		namespace Metrics
		{
			/// <summary>
			/// TaskLevelProfiling: Scheduler loop with full per-task profiling and timing metrics.
			/// Implements Profiling::ITaskLevelProfiler for metrics retrieval.
			/// 
			/// Collects detailed timing metrics for each individual task plus system metrics:
			/// - Per-task execution time (cumulative duration)
			/// - Per-task maximum execution time (worst-case spike)
			/// - Per-task iteration count (how many times each task ran)
			/// - Total idle sleep time
			/// - Total idle time
			/// - Total trace time
			/// 
			/// Profiling data is accumulated until a profile is requested with RequestMetrics().
			/// The result is delivered asynchronously to the supplied listener at the end of
			/// a scheduler loop iteration, after which the accumulated metrics are cleared.
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
			/// - Task level granularity vs system only
			/// 
			/// Handles dynamic task count changes gracefully by detecting mismatches
			/// and resetting trace data to prevent stale or inconsistent statistics.
			/// 
			/// Usage:
			/// Call Loop() as frequently as possible (typically in main loop).
			/// To receive traces, periodically call RequestMetrics() with an ITaskLevelListener implementation.
			/// </summary>
			/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
			/// <typeparam name="IdleSleepEnabled">Enable low-power idle sleep when no tasks are running.</typeparam>
			template<task_index_t MaxTaskCount, bool IdleSleepEnabled = false>
			class TaskLevelProfiling : public ITaskLevelProfiler, public AbstractScheduler<MaxTaskCount>
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
				TaskMetrics TaskTraces[MaxTaskCount]{};

				/// <summary>
				/// Global profiling trace for the current measurement window.
				/// Includes total scheduling overhead, idle sleep time, iteration count, and task count.
				/// Reset to zero after the trace listener is notified.
				/// </summary>
				SystemMetrics System{};
				ITaskLevelListener* ResultListener = nullptr;

				task_index_t TaskTraceCount = 0;

			public:
				TaskLevelProfiling()
					: ITaskLevelProfiler()
					, Base(IdleSleepEnabled)
				{}

				/// <summary>
				/// Requests accumulated profiling data for all tasks and global metrics
				/// asynchronously. The result is delivered to the supplied listener at the
				/// end of a scheduler loop iteration, after which the profiling data is
				/// cleared and a new measurement window begins.
				/// </summary>
				/// <param name="resultListener">Listener that receives the global trace and per-task trace array.</param>
				bool RequestMetrics(ITaskLevelListener* resultListener) override
				{
					ResultListener = resultListener;
					return ResultListener != nullptr;
				}

				/// <summary>
				/// Resets all profiling counters (global and per-task) to zero.
				/// Called automatically after the trace listener is notified.
				/// Also called automatically when task count changes to prevent stale data.
				/// Can be called manually to discard accumulated data and start a fresh measurement window.
				/// </summary>
				void ResetMetrics() override
				{
					System = SystemMetrics{};

					for (task_index_t i = 0; i < MaxTaskCount; i++)
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
					ResetMetrics();
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
				/// - System.Scheduling: Cumulative scheduler overhead excluding task execution and sleep (microseconds)
				/// - System.IdleSleep: Cumulative time spent in idle sleep (microseconds)
				/// - System.Iterations: Number of Loop() calls (scheduler tick count)
				/// - System.TaskCount: Number of active tasks (snapshot at trace window start)
				/// 
				/// Profiling measurements (per-task):
				/// - TaskTraces[i].Duration: Cumulative execution time for task i (microseconds)
				/// - TaskTraces[i].MaxDuration: Worst-case execution time for task i (microseconds)
				/// - TaskTraces[i].Iterations: Number of times task i executed
				/// 
				/// Note: Sum of TaskTraces[].Duration equals total busy time (task execution).
				///       System.Scheduling includes all task execution plus scheduler dispatch overhead.
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
					uint32_t measure = 0; // Reusable timestamp for measuring individual task segments.
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

					// Detect task count changes and reset trace data if necessary to prevent stale statistics.
					if (TaskTraceCount != TaskCount)
					{
						ResetMetrics();
						TaskTraceCount = TaskCount;
						measure = Platform::GetProfilerTimestamp();
					}

					// Reset per-iteration activity before dispatch. Task execution and,
					// when enabled, registry mutations set Hot again during this iteration.
					Hot = false;

					// Run all tasks that are due, measuring each task's execution time individually.
					for (task_index_t i = 0; i < TaskCount; i++)
					{
						if (Tasks[i].RunIfTime())
						{
							const uint32_t elapsed = Platform::GetProfilerTimestamp() - measure;
							measure += elapsed;

							// Optimization: under running load, skip idle sleep checks.
							Hot = true;

							TaskTraces[i].Iterations++;
							TaskTraces[i].Duration += elapsed; // Append task execution time to cumulative duration.
							System.Busy += elapsed; // Append task busy time.
							System.Scheduling -= elapsed; // Subtract task busy time from scheduling overhead.

							if (elapsed > TaskTraces[i].MaxDuration)
							{
								TaskTraces[i].MaxDuration = elapsed;
							}
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

					System.Scheduling += Platform::GetProfilerTimestamp() - loopStart;
				}

				void Loop(ConditionalDispatch::FalseType)
				{
					uint32_t measure = 0; // Reusable timestamp for measuring individual task segments.
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

					// Detect task count changes and reset trace data if necessary to prevent stale statistics.
					if (TaskTraceCount != TaskCount)
					{
						ResetMetrics();
						TaskTraceCount = TaskCount;
						measure = Platform::GetProfilerTimestamp();
					}

					// Run all tasks that are due, measuring each task's execution time individually.
					for (task_index_t i = 0; i < TaskCount; i++)
					{
						if (Tasks[i].RunIfTime())
						{
							const uint32_t elapsed = Platform::GetProfilerTimestamp() - measure;
							measure += elapsed;

							TaskTraces[i].Iterations++;
							TaskTraces[i].Duration += elapsed;
							System.Busy += elapsed;
							if (elapsed > TaskTraces[i].MaxDuration)
							{
								TaskTraces[i].MaxDuration = elapsed;
							}
						}
					}

					System.Scheduling += Platform::GetProfilerTimestamp() - loopStart;
				}


			private:
				void NotifyProfileResult()
				{
					auto* listener = ResultListener;
					ResultListener = nullptr;

					listener->OnMetricsResult(System, TaskTraces, TaskTraceCount);
					ResetMetrics();
				}
			};
		}
	}

	// Alias for the task-level metrics profiling scheduler.
	template<task_index_t MaxTaskCount, bool IdleSleepEnabled>
	using SchedulerMetricsTaskLevel = Profiling::Metrics::TaskLevelProfiling<MaxTaskCount, IdleSleepEnabled>;
}
#endif