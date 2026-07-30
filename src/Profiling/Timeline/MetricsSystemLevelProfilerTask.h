#ifndef _HARMONIC_PROFILING_TIMELINE_METRICS_SYSTEM_LEVEL_PROFILER_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_METRICS_SYSTEM_LEVEL_PROFILER_TASK_h

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
			/// Aggregates timeline trace samples into a system-level metrics result.
			/// Implements the same interface as metrics::SystemLevelProfilerTask, but uses timeline traces instead of full profiler traces.
			/// </summary>
			/// <typeparam name="MaxTaskCount"></typeparam>
			template<uint8_t MaxTaskCount>
			class MetricsSystemLevelProfilerTask : public ITask
				, public Timeline::ISystemLevelListener
				, public Metrics::ISystemLevelProfiler
			{
			private:
				static constexpr uint32_t TraceMaxPeriod = 10;
			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				Timeline::ISystemLevelProfiler& Profiler;

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
				/// Global profiling trace for the current measurement window.
				/// Includes total scheduling overhead, idle sleep time, and task count.
				/// Reset to zero after the trace listener is notified.
				/// </summary>
				SystemMetrics System{};
				SystemTimelineSample PreviousSample{};
				bool HasPreviousSample = false;
				Metrics::ISystemLevelListener* ResultListener = nullptr;

			public:
				MetricsSystemLevelProfilerTask(TaskRegistry& registry, Timeline::ISystemLevelProfiler& profiler)
					: ITask()
					, Profiler(profiler)
					, Registry(registry)
				{}

				bool RequestMetrics(Metrics::ISystemLevelListener* resultListener) override
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

				void OnTimelineResult(const SystemTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						const SystemTimelineSample& current = samples[i];
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
							else if (PreviousSample.Handle == TRACE_ACTIVE_HANDLE)
							{
								System.Busy += duration;
							}
							else
							{
								// Unknown, treat as scheduler overhead.
								System.Scheduling += duration;
							}
						}

						PreviousSample = current;
						HasPreviousSample = true;
					}

					Registry.SetPeriodAndEnabled(Handle, 0, true);
				}

				void Run() override
				{
					// Forward current trace result to the requested listener.
					if (ResultListener != nullptr)
					{
						auto* listener = ResultListener;
						ResultListener = nullptr;
						listener->OnMetricsResult(System);
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
					PreviousSample = {};
					HasPreviousSample = false;
					Profiler.ResetTimeline();
				}
			};

		}

	}

}
#endif