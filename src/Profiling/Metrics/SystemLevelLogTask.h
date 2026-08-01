#ifndef _HARMONIC_PROFILING_METRICS_SYSTEM_LEVEL_LOG_TASK_h
#define _HARMONIC_PROFILING_METRICS_SYSTEM_LEVEL_LOG_TASK_h

#include "../Logging.h"
#include "../../Task/DynamicTask.h"

#include <Print.h>

namespace Harmonic
{
	namespace Profiling
	{
		namespace Metrics
		{
			template<uint8_t MaxTaskCount, uint32_t LogPeriod>
			class SystemLevelLogTask : public ISystemLevelListener, public DynamicTask
			{
			private:
				/// <summary>
				/// A reference to a Print object used for serial output.
				/// </summary>
				Print& Output;

				/// <summary>
				/// Profiler source reference.
				/// </summary>
				ISystemLevelProfiler& Profiler;

			private:
				SystemMetrics System{};
				uint32_t LastTraceRequest = 0;
				bool ResultPending = false;

			public:
				SystemLevelLogTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, Print& output)
					: ISystemLevelListener()
					, DynamicTask(registry)
					, Output(output)
					, Profiler(profiler)
				{}

				virtual void OnMetricsResult(const SystemMetrics& systemMetrics) override
				{
					memcpy(&System, &systemMetrics, sizeof(SystemMetrics));
					WakeNow(); // Wake-up the log task to process the trace result immediately.
					ResultPending = true;
				}

				void Run() override
				{
					if (ResultPending)
					{
						ResultPending = false;
						Logging::PrintSchedulerMetrics(Output, System);
					}

					const uint32_t elapsed = Platform::GetTimestamp() - LastTraceRequest;

					if (elapsed >= LogPeriod)
					{
						LastTraceRequest = Platform::GetTimestamp();
						Profiler.RequestMetrics(this);
						SetDelayFromNow(LogPeriod);
					}
					else
					{
						// Request periodic trace from the profiler, regardless of whether the current trace was valid or not.
						SetDelayFromNow(LogPeriod - elapsed);
					}
				}

				/// <summary>
				/// Starts the system level log task.
				/// </summary>
				/// <param name="nameProvider">UNUSED task name provider. Only here to match the interface.</param>
				/// <returns>True if the task was successfully started; otherwise, false.</returns>
				bool Start(ITaskNameProvider* /*nameProvider*/ = nullptr)
				{	
					if (Attach(LogPeriod, true))
					{
						LastTraceRequest = Platform::GetTimestamp();

						return true;
					}

					return false;
				}

				void Stop()
				{
					Detach();
				}
			};
		}
	}
}
#endif