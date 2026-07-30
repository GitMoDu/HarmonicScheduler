#ifndef _HARMONIC_PROFILING_LOGGING_h
#define _HARMONIC_PROFILING_LOGGING_h

#include "../Model/Profiling.h"

#include <Print.h>

namespace Harmonic
{
	namespace Profiling
	{
		namespace Logging
		{
			inline void PrintLogHeader(Print& output)
			{
				output.println(F("ID\t\tCPU(%)\tCALLS\tTIME(us)\tMAX(us)"));
			}

			inline void PrintTagScheduler(Print& output)
			{
				output.print(F("BUSY\t"));
			}

			inline void PrintTagIdle(Print& output)
			{
				output.print(F("IDLE\t"));
			}

			inline void PrintTagSleep(Print& output)
			{
				output.print(F("SLEEP\t"));
			}

			inline void PrintTimelineStart(Print& output)
			{
				output.println(F("START"));
			}

			inline void PrintTimelineEntry(Print& output, const SystemTimelineSample& sample)
			{
				const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&sample.Timestamp);

				// Big-endian transmission (MSB first)
				output.write(bytes[3]);
				output.write(bytes[2]);
				output.write(bytes[1]);
				output.write(bytes[0]);

				output.write(sample.Handle);
			}

			inline void PrintTimelineEntry(Print& output, const TaskTimelineSample& sample)
			{
				const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&sample.Timestamp);

				// Big-endian transmission (MSB first)
				output.write(bytes[3]);
				output.write(bytes[2]);
				output.write(bytes[1]);
				output.write(bytes[0]);

				output.write(sample.Handle);
			}

			inline void PrintTimelineTaskLevelHeader(Print& output)
			{
				output.println();
				output.println(F("TIMELINE TRACE"));

				output.println(F("Handle\tTask"));

				output.print(TASK_INVALID_HANDLE);
				output.println(F("\tSCHEDULER"));
				output.print(TRACE_SLEEP_HANDLE);
				output.println(F("\tSLEEP"));
				output.print(TRACE_TASK_HANDLE);
				output.println(F("\tTRACE"));
			}

			inline void PrintTimelineSystemLevelHeader(Print& output)
			{
				output.println();
				output.println(F("TIMELINE TRACE"));

				output.println(F("Handle\tTask"));

				output.print(TASK_INVALID_HANDLE);
				output.println(F("\tSCHEDULER"));
				output.print(TRACE_SLEEP_HANDLE);
				output.println(F("\tSLEEP"));
				output.print(TRACE_ACTIVE_HANDLE);
				output.println(F("\tACTIVE"));
			}

			inline void PrintTimelineTaskNames(Print& output, Profiling::ITaskNameProvider* nameProvider)
			{
				if (nameProvider != nullptr)
				{
					for (size_t i = 0; i < TASK_MAX_COUNT; i++)
					{
						if (nameProvider->IsTaskKnown(i))
						{
							output.print(i);
							output.print('\t');
							output.println(nameProvider->GetTaskName(i));
						}
					}
				}
			}

			inline size_t PrintTagLog(Print& output)
			{
				return output.print(F("LOG"));
			}

			inline size_t PrintTagGenericTask(Print& output, const task_handle_t handle)
			{
				return output.print(F("Task")) + output.print(handle);
			}

			inline void PrintSeparator(Print& output)
			{
				for (uint_fast8_t i = 0; i < 55; i++)
				{
					output.print('-');
				}
				output.println();
			}

			static void PrintSchedulerMetrics(Print& output, const Profiling::SystemMetrics& metrics)
			{
				// Sum up total metrics time.
				const uint32_t metricsTime = metrics.Scheduling + metrics.Busy + metrics.IdleSleep;

				// Calculate percentages.
				const uint8_t cpu = (metricsTime > 0)
					? static_cast<uint8_t>((metrics.Busy * 100) / metricsTime) : 0U;
				const uint8_t sleep = (metricsTime > 0)
					? static_cast<uint8_t>((metrics.IdleSleep * 100) / metricsTime) : 0U;
				const uint8_t idle = (metricsTime > 0)
					? static_cast<uint8_t>((metrics.Scheduling * 100) / metricsTime) : 0U;

				output.println();
				PrintLogHeader(output);
				PrintTagScheduler(output);
				output.print('\t');
				output.print(cpu);
				output.print('\t');

				output.print('\t');
				output.print(metrics.Busy);
				output.print('\t');
				output.print('\t');
				output.println(metricsTime);

				PrintTagIdle(output);
				output.print('\t');
				output.print(idle);
				output.print('\t');
				output.print('\t');
				output.println(metrics.Scheduling);

				PrintTagSleep(output);
				output.print('\t');
				output.print(sleep);
				output.print('\t');
				output.print('\t');
				output.println(metrics.IdleSleep);

				PrintSeparator(output);
			}
		}
	}
}
#endif