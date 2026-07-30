#ifndef _HARMONIC_PROFILING_TIMELINE_METRICS_TASK_LEVEL_PROFILER_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_METRICS_TASK_LEVEL_PROFILER_TASK_h	

#include "../../Model/ITask.h"
#include "../../Model/Profiling.h"
#include "../../Model/TaskRegistry.h"

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
			class MetricsTaskLevelProfilerTask : public ITask
				, public ::Harmonic::Profiling::Timeline::ITaskLevelListener
				, public ::Harmonic::Profiling::Metrics::ITaskLevelProfiler
			{
			private:
				static constexpr uint32_t TraceMaxPeriod = 10;
			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				::Harmonic::Profiling::Timeline::ITaskLevelProfiler& Profiler;

				/// <summary>
				/// Reference to the registry for managing this task.
				/// </summary>
				TaskRegistry& Registry;

			protected:
				/// <summary>
				/// Handle for the current registry attachment.
				/// Stable while attached; TASK_INVALID_HANDLE if unregistered. Handle
				/// values may be recycled after removal and are not lifetime-unique.
				/// </summary>
				task_handle_t Handle = TASK_INVALID_HANDLE;

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
				::Harmonic::Profiling::Metrics::ITaskLevelListener* ResultListener = nullptr;

			public:
				MetricsTaskLevelProfilerTask(TaskRegistry& registry, ::Harmonic::Profiling::Timeline::ITaskLevelProfiler& profiler)
					: ITask()
					, ::Harmonic::Profiling::Metrics::ITaskLevelProfiler()
					, Profiler(profiler)
					, Registry(registry)
				{}

				bool RequestMetrics(::Harmonic::Profiling::Metrics::ITaskLevelListener* resultListener) override
				{
					ResultListener = resultListener;
					if (ResultListener != nullptr)
					{
						Registry.SetPeriodAndEnabled(Handle, 0, true);

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

					Registry.SetPeriodAndEnabled(Handle, 0, true);
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
						Registry.SetPeriodAndEnabled(Handle, 0, true);
					}
					else
					{
						Registry.SetEnabled(Handle, false);
					}
				}

				bool Start()
				{
					const task_handle_t handle = Registry.Attach(this, 0, false);
					if (handle != TASK_INVALID_HANDLE)
					{
						Handle = handle;

						return Profiler.SetTimelineListener(this);
					}

					return false;
				}

				void Stop()
				{
					if (Registry.Detach(Handle))
					{
						Handle = TASK_INVALID_HANDLE;
						Profiler.SetTimelineListener(nullptr);
					}
				}

				task_handle_t GetHandle() const
				{
					return Handle;
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