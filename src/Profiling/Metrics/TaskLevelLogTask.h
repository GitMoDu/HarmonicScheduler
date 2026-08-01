#ifndef _HARMONIC_PROFILING_METRICS_TASK_LEVEL_LOG_TASK_h
#define _HARMONIC_PROFILING_METRICS_TASK_LEVEL_LOG_TASK_h

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
			class TaskLevelLogTask : public ITaskLevelListener, public DynamicTask
			{
			private:
				/// <summary>
				/// A reference to a Print object used for serial output.
				/// </summary>
				Print& Output;

				/// <summary>
				/// Profiler source reference.
				/// </summary>
				ITaskLevelProfiler& Profiler;

			private:
				ITaskNameProvider* NameProvider = nullptr;
				uint32_t LastTraceRequest = 0;

				SystemMetrics System{};
				TaskMetrics Traces[MaxTaskCount]{};
				task_handle_t TaskTraceCount = 0;

				bool ResultPending = false;

			public:
				TaskLevelLogTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, Print& output)
					: ITaskLevelListener()
					, DynamicTask(registry)
					, Output(output)
					, Profiler(profiler)
				{}


				void OnMetricsResult(const SystemMetrics& systemMetrics, const TaskMetrics* tasksMetrics, const task_index_t taskMetricsCount) override
				{
					memcpy(&System, &systemMetrics, sizeof(SystemMetrics));
					const task_handle_t copyCount = (taskMetricsCount < MaxTaskCount) ? taskMetricsCount : MaxTaskCount;
					memcpy(Traces, tasksMetrics, copyCount * sizeof(TaskMetrics));
					TaskTraceCount = copyCount;
					ResultPending = true;
					WakeNow(); // Wake-up the log task to process the trace result immediately.
				}

				void Run() override
				{
					if (ResultPending)
					{
						ResultPending = false;

						// Sum up total trace time.
						const uint32_t traceTime = System.Scheduling
							+ System.Busy
							+ System.IdleSleep;

						Logging::PrintSchedulerMetrics(Output, System);

						for (task_index_t i = 0; i < TaskTraceCount; i++)
						{
							const uint8_t task = (traceTime > 0U)
								? static_cast<uint8_t>((Traces[i].Duration * 100U) / traceTime)
								: 0U;

							size_t charCount = 0;

							if (NameProvider == nullptr
								&& Traces[i].Handle == Handle)
							{
								// If no name provider is available, at least highlight the log task itself in the output.
								charCount = Logging::PrintTagLog(Output);
							}
							else
							{
								if (NameProvider != nullptr && NameProvider->IsTaskKnown(Traces[i].Handle))
								{
									// Print the known task name.
									charCount = Output.print(NameProvider->GetTaskName(Traces[i].Handle));
								}
								else
								{
									// Print the task handle as a generic task.
									charCount = Logging::PrintTagGenericTask(Output, Traces[i].Handle);
								}
							}

							// Pad the task name to align the output columns.
							if (charCount < 8)
							{
								Output.print('\t');
							}

							Output.print('\t');
							Output.print(task);
							Output.print('\t');
							Output.print(Traces[i].Iterations);
							Output.print('\t');
							Output.print(Traces[i].Duration);
							Output.print('\t');
							Output.print('\t');
							Output.println(Traces[i].MaxDuration);
						}
						Output.println();
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

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Attach(LogPeriod, true))
					{
						NameProvider = nameProvider;
						LastTraceRequest = Platform::GetTimestamp();

						return true;
					}

					return false;
				}

				void Stop()
				{
					Detach();
				}

				task_handle_t GetTaskHandle() const
				{
					return Handle;
				}
			};
		}
	}
}
#endif