#ifndef _HARMONIC_PROFILING_TIMELINE_METRICS_TASK_LEVEL_PROFILER_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_METRICS_TASK_LEVEL_PROFILER_TASK_h	

#include "../../Model/Profiling.h"
#include "../../Task/AbstractTask.h"

#include <Print.h>

namespace Harmonic
{
	namespace Profiling
	{
		namespace Timeline
		{
			/// <summary>
			/// Aggregates timeline trace samples into a task-level metrics result.
			/// Implements the same interface as metrics::TaskLevelLogTask, but uses timeline traces instead of full profiler traces.
			/// </summary>
			/// <typeparam name="MaxTaskCount"></typeparam>
			template<uint8_t MaxTaskCount>
			class MetricsTaskLevelProfilerTask
				: public Timeline::ITaskLevelListener
				, public Metrics::ITaskLevelProfiler
				, public AbstractTask
			{
			private:
				static constexpr uint32_t TraceMaxPeriod = 10;

			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				Timeline::ITaskLevelProfiler& Profiler;

			private:
				/// <summary>
				/// Per-task profiling data array, indexed by task ID.
				/// Stores cumulative duration, max duration, and iteration count for each task.
				/// Reset to zero after the trace listener is notified.
				/// </summary>
				TaskMetrics TaskTraces[MaxTaskCount]{};
				uint32_t TraceDuration = 0;
				task_index_t TaskTraceCount = 0;
				TaskTimelineSample PreviousSample{};
				bool HasPreviousSample = false;

				/// <summary>
				/// Global profiling trace for the current measurement window.
				/// Includes total scheduling overhead, idle sleep time, iteration count, and task count.
				/// Reset to zero after the trace listener is notified.
				/// </summary>
				SystemMetrics System{};
				Metrics::ITaskLevelListener* ResultListener = nullptr;

			public:
				MetricsTaskLevelProfilerTask(TaskRegistry& registry, Timeline::ITaskLevelProfiler& profiler)
					: Timeline::ITaskLevelListener()
					, Metrics::ITaskLevelProfiler()
					, AbstractTask(registry)
					, Profiler(profiler)
				{}

				bool RequestMetrics(Metrics::ITaskLevelListener* resultListener) override
				{
					ResultListener = resultListener;
					if (ResultListener != nullptr)
					{
						SetEnabled(true);

						return true;
					}
					else
					{
						return false;
					}
				}

				void ResetMetrics() override
				{
					ClearTraceData();
				}

				void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					const uint32_t traceTime = Platform::GetProfilerTimestamp();

					for (size_t i = 0; i < sampleCount; i++)
					{
						const TaskTimelineSample& current = samples[i];
						if (HasPreviousSample)
						{
							const uint32_t duration = current.Timestamp - PreviousSample.Timestamp;

							if (PreviousSample.Handle == TRACE_SLEEP_HANDLE)
							{
								System.IdleSleep += duration;
							}
							else if (PreviousSample.Handle == TRACE_SCHEDULER_HANDLE)
							{
								System.Scheduling += duration;
							}
							else if (PreviousSample.Handle == TRACE_TASK_HANDLE)
							{
								System.Scheduling += duration;
							}
							else
							{
								AppendTaskTrace(duration, PreviousSample.Handle);
							}
						}

						PreviousSample = current;
						HasPreviousSample = true;
					}

					SetEnabled(true);
					TraceDuration += Platform::GetProfilerTimestamp() - traceTime;
				}

				void Run() override
				{
					// Forward current trace result to the requested listener.
					if (ResultListener != nullptr)
					{
						auto* listener = ResultListener;
						ResultListener = nullptr;

						for (task_index_t i = 0; i < TaskTraceCount; i++)
						{
							if (TaskTraces[i].Handle == Handle)
							{
								TaskTraces[i].Duration = TraceDuration;
								break;
							}
						}

						listener->OnMetricsResult(System, TaskTraces, TaskTraceCount);
						ClearTraceData();
						SetEnabled(true);
					}
					else
					{
						SetEnabled(false);
					}
				}

				bool Start()
				{
					if (Attach(0, false))
					{
						return Profiler.SetTimelineListener(this);
					}

					return false;
				}

				void Stop()
				{
					Detach();
					Profiler.SetTimelineListener(nullptr);
				}

			private:
				void ClearTraceData()
				{
					System = {};
					TaskTraceCount = 0;
					TraceDuration = 0;
					PreviousSample = {};
					HasPreviousSample = false;
					Profiler.ResetTimeline();
					for (task_index_t i = 0; i < MaxTaskCount; i++)
					{
						TaskTraces[i].Handle = TASK_INVALID_HANDLE;
					}
				}

				void AppendTaskTrace(const uint32_t duration, const task_handle_t handle)
				{
					System.Busy += duration;

					if (handle != TASK_INVALID_HANDLE)
					{
						TaskMetrics* taskTrace = FindTaskTrace(handle);
						if (taskTrace != nullptr)
						{
							taskTrace->Duration += duration;
							taskTrace->Iterations++;
							if (duration > taskTrace->MaxDuration)
							{
								taskTrace->MaxDuration = duration;
							}
						}
					}
				}

				TaskMetrics* FindTaskTrace(const task_handle_t handle)
				{
					for (task_index_t i = 0; i < TaskTraceCount; i++)
					{
						if (TaskTraces[i].Handle == handle)
						{
							return &TaskTraces[i];
						}
					}

					if (TaskTraceCount >= MaxTaskCount)
					{
						return nullptr;
					}

					task_index_t insertionIndex = 0;
					while (insertionIndex < TaskTraceCount
						&& TaskTraces[insertionIndex].Handle < handle)
					{
						insertionIndex++;
					}

					for (task_index_t i = TaskTraceCount; i > insertionIndex; i--)
					{
						TaskTraces[i] = TaskTraces[i - 1];
					}

					TaskTraceCount++;
					TaskMetrics& taskTrace = TaskTraces[insertionIndex];
					taskTrace = {};
					taskTrace.Handle = handle;
					return &taskTrace;
				}
			};

		}

	}

}
#endif