#ifndef _HARMONIC_PROFILING_METRICS_TASK_LEVEL_LOG_TASK_h
#define _HARMONIC_PROFILING_METRICS_TASK_LEVEL_LOG_TASK_h

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
			class TaskLevelLogTask : public ITask, public ITaskLevelListener
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
				ITaskNameProvider* NameProvider = nullptr;
				uint32_t LastTraceRequest = 0;

				SystemMetrics System{};
				TaskMetrics Traces[MaxTaskCount]{};
				task_handle_t TaskTraceCount = 0;

				bool ResultPending = false;

			public:
				TaskLevelLogTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, Print& output)
					: ITask()
					, ITaskLevelListener()
					, Output(output)
					, Profiler(profiler)
					, Registry(registry)
				{}


				void OnMetricsResult(const SystemMetrics& systemMetrics, const TaskMetrics* tasksMetrics, const task_index_t taskMetricsCount) override
				{
					memcpy(&System, &systemMetrics, sizeof(SystemMetrics));
					const task_handle_t copyCount = (taskMetricsCount < MaxTaskCount) ? taskMetricsCount : MaxTaskCount;
					memcpy(Traces, tasksMetrics, copyCount * sizeof(TaskMetrics));
					TaskTraceCount = copyCount;
					ResultPending = true;
					Registry.SetPeriod(Handle, 0); // Wake-up the log task to process the trace result immediately.
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
						Registry.SetPeriod(Handle, LogPeriod);
					}
					else
					{
						// Request periodic trace from the profiler, regardless of whether the current trace was valid or not.
						Registry.SetPeriod(Handle, LogPeriod - elapsed);
					}
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					const task_handle_t handle = Registry.Attach(this, LogPeriod, true);
					if (handle != TASK_INVALID_HANDLE)
					{
						NameProvider = nameProvider;
						Handle = handle;
						LastTraceRequest = Platform::GetTimestamp();

						return true;
					}

					return false;
				}

				void Stop()
				{
					if (Registry.Detach(Handle))
					{
						Handle = TASK_INVALID_HANDLE;
					}
				}

				task_handle_t GetHandle() const
				{
					return Handle;
				}
			};
		}
	}
}
#endif