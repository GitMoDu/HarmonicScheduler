#ifndef _HARMONIC_PROFILING_TIMELINE_METRICS_SYSTEM_LEVEL_PROFILER_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_METRICS_SYSTEM_LEVEL_PROFILER_TASK_h

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
			/// Aggregates timeline trace samples into a system-level metrics result.
			/// Implements the same interface as metrics::SystemLevelProfilerTask, but uses timeline traces instead of full profiler traces.
			/// </summary>
			/// <typeparam name="MaxTaskCount"></typeparam>
			template<uint8_t MaxTaskCount>
			class MetricsSystemLevelProfilerTask
				: public Timeline::ISystemLevelListener
				, public Metrics::ISystemLevelProfiler
				, public AbstractTask
			{
			private:
				static constexpr uint32_t TraceMaxPeriod = 10;

			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				Timeline::ISystemLevelProfiler& Profiler;

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
					: Timeline::ISystemLevelListener()
					, Metrics::ISystemLevelProfiler()
					, AbstractTask(registry)
					, Profiler(profiler)
				{}

				bool RequestMetrics(Metrics::ISystemLevelListener* resultListener) override
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

					SetEnabled(true);
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
					PreviousSample = {};
					HasPreviousSample = false;
					Profiler.ResetTimeline();
				}
			};

		}

	}

}
#endif