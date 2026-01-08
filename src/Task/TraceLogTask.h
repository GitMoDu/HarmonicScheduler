#ifndef _HARMONIC_TRACE_LOG_TASK_h
#define _HARMONIC_TRACE_LOG_TASK_h

#include "../Model/ITask.h"
#include "../Model/Profiling.h"
#include "../Model/TaskRegistry.h"

#include <Print.h>

namespace Harmonic
{
	namespace TraceLogging
	{
		static void PrintLogHeader(Print& output)
		{
			output.println(F("ID\tCPU(%)\tCALLS\tTIME(us)\tMAX(us)"));
		}

		static void PrintTagScheduler(Print& output)
		{
			output.print(F("BUSY"));
		}

		static void PrintTagIdle(Print& output)
		{
			output.print(F("IDLE"));
		}

		static void PrintTagSleep(Print& output)
		{
			output.print(F("SLEEP"));
		}

		static void PrintTagLog(Print& output)
		{
			output.print(F("Log"));
		}

		static void PrintSeparator(Print& output)
		{
			for (uint_fast8_t i = 0; i < 47; i++)
			{
				output.print('-');
			}
			output.println();
		}
	}

	template<uint8_t MaxTaskCount, ProfileLevelEnum Level, uint32_t LogPeriod>
	class MockTraceLogTask
	{
	public:
		MockTraceLogTask(TaskRegistry& registry, TaskRegistry& mockProfiler, Print& output)
		{
		}

		bool Start()
		{
			return true;
		}

		void Stop()
		{
		}
	};

	template<uint8_t MaxTaskCount, ProfileLevelEnum Level, uint32_t LogPeriod>
	class BaseTraceLogTask : public ITask
	{
	private:
		/// <summary>
		/// A reference to a Print object used for serial output.
		/// </summary>
		Print& Output;

		/// <summary>
		/// Profiler source reference.
		/// </summary>
		Profiling::IBaseProfiler& Profiler;

		/// <summary>
		/// Reference to the registry for managing this task.
		/// </summary>
		TaskRegistry& Registry;

		/// <summary>
		/// Unique identifier for this task within the registry.
		/// Set during registration; TASK_INVALID_ID if unregistered.
		/// </summary>
		volatile task_id_t Id = TASK_INVALID_ID;

	private:
		Profiling::BaseTrace Trace{};

		int32_t LastLogDuration = INT32_MIN;
		uint32_t MaxTraceDuration = 0;

	public:
		/// <summary>
		/// Constructs a basic trace log task with a reference to the registry.
		/// </summary>
		BaseTraceLogTask(TaskRegistry& registry, Profiling::IBaseProfiler& profiler, Print& output)
			: ITask()
			, Output(output)
			, Profiler(profiler)
			, Registry(registry)
		{
		}

		void Run() override
		{
			const uint32_t traceStart = Platform::GetProfilerTimestamp();
			if (Profiler.GetTrace(Trace))
			{
				// Busy time from trace.
				const uint32_t busyTime = Trace.Busy;

				// Sum up total trace time.
				const uint32_t traceTime = Trace.Scheduling + Trace.IdleSleep;

				// Calculate idle time.
				const uint32_t idleTime = Trace.Scheduling - busyTime;

				// Calculate percentages.
				const uint8_t cpu = (traceTime > 0)
					? static_cast<uint8_t>((busyTime * 100) / traceTime) : 0U;
				const uint8_t sleep = (traceTime > 0)
					? static_cast<uint8_t>((Trace.IdleSleep * 100) / traceTime) : 0U;
				const uint8_t idle = (traceTime > 0)
					? static_cast<uint8_t>((idleTime * 100) / traceTime) : 0U;
				const uint8_t log = (traceTime > 0)
					? static_cast<uint8_t>((LastLogDuration * 100) / traceTime) : 0U;

				Output.println();
				TraceLogging::PrintLogHeader(Output);
				TraceLogging::PrintTagScheduler(Output);
				Output.print('\t');
				Output.print(cpu);
				Output.print('\t');
				Output.print(Trace.Iterations);
				Output.print('\t');
				Output.print(busyTime);
				Output.print('\t');
				Output.print('\t');
				Output.println(traceTime);

				TraceLogging::PrintTagIdle(Output);
				Output.print('\t');
				Output.println(idle);

				TraceLogging::PrintTagSleep(Output);
				Output.print('\t');
				Output.print(sleep);
				Output.print(F("\t\t"));
				Output.println(Trace.IdleSleep);

				TraceLogging::PrintSeparator(Output);

				TraceLogging::PrintTagLog(Output);
				Output.print('\t');
				Output.print(log);
				Output.print('\t');
				Output.print(1); // Fixed single log call.
				Output.print('\t');

				if (LastLogDuration >= 0)
				{
					Output.print(LastLogDuration);
					Output.print('\t');
					Output.print('\t');
					Output.println(MaxTraceDuration);
				}
				else
				{
					// Approximate first log duration.
					const uint32_t firstDuration = Platform::GetProfilerTimestamp() - traceStart;
					Output.print(firstDuration);
					Output.print('\t');
					Output.print('\t');
					Output.println(firstDuration);
				}

				LastLogDuration = Platform::GetProfilerTimestamp() - traceStart;
				if (LastLogDuration > static_cast<int32_t>(MaxTraceDuration))
				{
					MaxTraceDuration = static_cast<uint32_t>(LastLogDuration);
				}
			}
		}

		bool Start()
		{
			return Registry.Attach(this, LogPeriod, true);
		}

		void Stop()
		{
			Registry.Detach(Id);
		}

		void OnTaskIdUpdated(const task_id_t taskId) final
		{
			// Store the assigned task ID for later use.
			Id = taskId;
		}
	};

	template<uint8_t MaxTaskCount, ProfileLevelEnum Level, uint32_t LogPeriod>
	class FullTraceLogTask : public ITask
	{
	private:
		/// <summary>
		/// A reference to a Print object used for serial output.
		/// </summary>
		Print& Output;

		/// <summary>
		/// Profiler source reference.
		/// </summary>
		Profiling::IFullProfiler& Profiler;

		/// <summary>
		/// Reference to the registry for managing this task.
		/// </summary>
		TaskRegistry& Registry;

		/// <summary>
		/// Unique identifier for this task within the registry.
		/// Set during registration; TASK_INVALID_ID if unregistered.
		/// </summary>
		volatile task_id_t Id = TASK_INVALID_ID;

	private:
		Profiling::TaskTrace Traces[MaxTaskCount]{};
		Profiling::FullTrace Trace{};

		uint32_t GetTracesDuration() const
		{
			uint32_t total = 0;
			for (uint8_t i = 0; i < Trace.TaskCount; i++)
			{
				total += Traces[i].Duration;
			}
			return total;
		}

	public:
		/// <summary>
		/// Constructs a ProfilerLogTask with a reference to the registry.
		/// </summary>
		FullTraceLogTask(TaskRegistry& registry, Profiling::IFullProfiler& profiler, Print& output)
			: ITask()
			, Output(output)
			, Profiler(profiler)
			, Registry(registry)
		{
		}

		void Run() override
		{
			if (Profiler.GetTrace(Trace, Traces, MaxTaskCount))
			{
				// Sum up total task run time.
				const uint32_t busyTime = GetTracesDuration();

				// Sum up total trace time.
				const uint32_t traceTime = Trace.Scheduling + Trace.IdleSleep;

				// Calculate idle time.
				const uint32_t idleTime = Trace.Scheduling - busyTime;

				// Calculate CPU usage percentage.
				const uint8_t cpu = (traceTime > 0)
					? static_cast<uint8_t>((busyTime * 100) / traceTime)
					: 0U;

				const uint8_t sleep = (traceTime > 0)
					? static_cast<uint8_t>((Trace.IdleSleep * 100) / traceTime)
					: 0U;

				const uint8_t idle = (traceTime > 0)
					? static_cast<uint8_t>((idleTime * 100) / traceTime)
					: 0U;

				Output.println();
				TraceLogging::PrintLogHeader(Output);
				TraceLogging::PrintTagScheduler(Output);
				Output.print('\t');
				Output.print(cpu);
				Output.print('\t');
				Output.print(Trace.Iterations);
				Output.print('\t');
				Output.print(busyTime);
				Output.print('\t');
				Output.print('\t');
				Output.println(traceTime);

				TraceLogging::PrintTagIdle(Output);
				Output.print('\t');
				Output.println(idle);

				TraceLogging::PrintTagSleep(Output);
				Output.print('\t');
				Output.print(sleep);
				Output.print('\t');
				Output.print('\t');
				Output.println(Trace.IdleSleep);

				TraceLogging::PrintSeparator(Output);

				for (uint_fast8_t i = 0; i < Trace.TaskCount; i++)
				{
					const uint8_t task = (traceTime > 0U)
						? static_cast<uint8_t>((Traces[i].Duration * 100U) / traceTime)
						: 0U;

					Output.println();
					if (i == Id)
					{
						TraceLogging::PrintTagLog(Output);
					}
					else
					{
						Output.print(F("Task"));
						Output.print(i);
					}
					Output.print('\t');
					Output.print(task);
					Output.print('\t');
					Output.print(Traces[i].Iterations);
					Output.print('\t');
					Output.print(Traces[i].Duration);
					Output.print('\t');
					Output.print('\t');
					Output.print(Traces[i].MaxDuration);
				}
				Output.println();
			}
		}

		bool Start()
		{
			return Registry.Attach(this, LogPeriod, true);
		}

		void Stop()
		{
			Registry.Detach(Id);
		}

		void OnTaskIdUpdated(const task_id_t taskId) final
		{
			// Store the assigned task ID for later use.
			Id = taskId;
		}
	};

	/// <summary>
	/// Template selector that maps ProfileLevelEnum to the appropriate trace log task type.
	/// - None  -> MockTraceLogTask (no-op logger)
	/// - Base  -> BaseTraceLogTask
	/// - Full  -> FullTraceLogTask
	/// </summary>
	template<uint8_t MaxTaskCount, ProfileLevelEnum Level, uint32_t LogPeriod>
	struct TraceLogTaskSelector;

	template<uint8_t MaxTaskCount, uint32_t LogPeriod>
	struct TraceLogTaskSelector<MaxTaskCount, ProfileLevelEnum::None, LogPeriod>
	{
		using Type = MockTraceLogTask<MaxTaskCount, ProfileLevelEnum::None, LogPeriod>;
	};

	template<uint8_t MaxTaskCount, uint32_t LogPeriod>
	struct TraceLogTaskSelector<MaxTaskCount, ProfileLevelEnum::Base, LogPeriod>
	{
		using Type = BaseTraceLogTask<MaxTaskCount, ProfileLevelEnum::Base, LogPeriod>;
	};

	template<uint8_t MaxTaskCount, uint32_t LogPeriod>
	struct TraceLogTaskSelector<MaxTaskCount, ProfileLevelEnum::Full, LogPeriod>
	{
		using Type = FullTraceLogTask<MaxTaskCount, ProfileLevelEnum::Full, LogPeriod>;
	};

	/// <summary>
	/// Convenience alias to obtain the trace log task type for a given ProfileLevelEnum.
	/// Example:
	///   using TraceLogger = TemplateTraceLogTask<MaxTaskCount, ProfileLevel, 1000>;
	/// </summary>
	template<uint8_t MaxTaskCount, ProfileLevelEnum Level, uint32_t LogPeriod>
	using TemplateTraceLogTask = typename TraceLogTaskSelector<MaxTaskCount, Level, LogPeriod>::Type;
}
#endif