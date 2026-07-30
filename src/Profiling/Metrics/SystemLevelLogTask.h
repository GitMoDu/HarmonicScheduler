#ifndef _HARMONIC_PROFILING_METRICS_SYSTEM_LEVEL_LOG_TASK_h
#define _HARMONIC_PROFILING_METRICS_SYSTEM_LEVEL_LOG_TASK_h

#include "../Logging.h"
#include "../../Model/ITask.h"
#include "../../Model/Profiling.h"
#include "../../Model/TaskRegistry.h"

#include <Print.h>

namespace Harmonic
{
	namespace Profiling
	{
		namespace Metrics
		{
			template<uint8_t MaxTaskCount, uint32_t LogPeriod>
			class SystemLevelLogTask : public ITask, public ISystemLevelListener
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

				/// <summary>
				/// Reference to the registry for managing this task.
				/// </summary>
				TaskRegistry& Registry;

				/// <summary>
				/// Handle for the current registry attachment.
				/// Stable while attached; TASK_INVALID_HANDLE if unregistered. Handle
				/// values may be recycled after removal and are not lifetime-unique.
				/// </summary>
				task_handle_t Handle = TASK_INVALID_HANDLE;

			private:
				SystemMetrics System{};
				uint32_t LastTraceRequest = 0;
				bool ResultPending = false;

			public:
				SystemLevelLogTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, Print& output)
					: ITask()
					, ISystemLevelListener()
					, Output(output)
					, Profiler(profiler)
					, Registry(registry)
				{}

				virtual void OnMetricsResult(const SystemMetrics& systemMetrics) override
				{
					memcpy(&System, &systemMetrics, sizeof(SystemMetrics));
					Registry.SetPeriod(Handle, 0); // Wake-up the log task to process the trace result immediately.
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
						Registry.SetPeriod(Handle, LogPeriod);
					}
					else
					{
						// Request periodic trace from the profiler, regardless of whether the current trace was valid or not.
						Registry.SetPeriod(Handle, LogPeriod - elapsed);
					}
				}

				bool Start()
				{
					const task_handle_t handle = Registry.Attach(this, LogPeriod, true);
					if (handle != TASK_INVALID_HANDLE)
					{
						Handle = handle;
						LastTraceRequest = Platform::GetTimestamp();

						return true;
					}

					return false;
				}

				task_handle_t GetHandle() const
				{
					return Handle;
				}

				void Stop()
				{
					if (Registry.Detach(Handle))
					{
						Handle = TASK_INVALID_HANDLE;
					}
				}
			};
		}
	}
}
#endif