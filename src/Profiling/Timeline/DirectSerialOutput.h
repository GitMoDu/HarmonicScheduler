#ifndef _HARMONIC_PROFILING_TIMELINE_DIRECT_SERIAL_OUTPUT_h
#define _HARMONIC_PROFILING_TIMELINE_DIRECT_SERIAL_OUTPUT_h

#include "../Logging.h"
#include "../../Model/Profiling.h"

namespace Harmonic
{
	namespace Profiling
	{
		namespace Timeline
		{
			/// <summary>
			/// Directly outputs timeline trace samples to a serial interface.
			/// Only suitable for small traces and fast, non-blocking serial interfaces.
			/// </summary>
			/// <typeparam name="SerialType"></typeparam>
			template<typename SerialType>
			class TaskLevelDirectSerialOutput : public ITaskLevelListener
			{
			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				ITaskLevelProfiler& Profiler;

				/// <summary>
				/// Serial output reference.
				/// </summary>
				SerialType& Output;

			public:
				TaskLevelDirectSerialOutput(ITaskLevelProfiler& profiler, SerialType& output)
					: ITaskLevelListener()
					, Profiler(profiler)
					, Output(output)
				{}

				void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						auto current = samples[i];
						Output.print(current.Timestamp);
						Output.print(',');
						Output.println(current.Handle);
					}
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Profiler.SetTimelineListener(this))
					{
						Logging::PrintTimelineTaskLevelHeader(Output);
						Logging::PrintTimelineTaskNames(Output, nameProvider);
						Logging::PrintTimelineStart(Output);

						return true;
					}

					return false;
				}

				void Stop()
				{
					Profiler.SetTimelineListener(nullptr);
				}
			};

			template<typename SerialType>
			class SystemLevelDirectSerialOutput : public ITaskLevelListener
			{
			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				ISystemLevelProfiler& Profiler;

				/// <summary>
				/// Serial output reference.
				/// </summary>
				SerialType& Output;

			public:
				SystemLevelDirectSerialOutput(ISystemLevelProfiler& profiler, SerialType& output)
					: ITaskLevelListener()
					, Profiler(profiler)
					, Output(output)
				{}

				void OnTimelineResult(const SystemTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						auto current = samples[i];
						Output.print(current.Timestamp);
						Output.print(',');
						Output.println(current.Handle);
					}
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Profiler.SetTimelineListener(this))
					{
						Logging::PrintTimelineSystemLevelHeader(Output);
						Logging::PrintTimelineStart(Output);

						return true;
					}

					return false;
				}

				void Stop()
				{
					Profiler.SetTimelineListener(nullptr);
				}
			};

			/// <summary>
			/// Template selector that maps ProfilerLevelEnum to the appropriate direct serial output type.
			/// </summary>
			template<ProfilerLevelEnum Level, typename SerialType>
			struct DirectSerialOutputSelector;

			template<typename SerialType>
			struct DirectSerialOutputSelector<ProfilerLevelEnum::System, SerialType>
			{
				using Type = SystemLevelDirectSerialOutput<SerialType>;
			};

			template<typename SerialType>
			struct DirectSerialOutputSelector<ProfilerLevelEnum::Task, SerialType>
			{
				using Type = TaskLevelDirectSerialOutput<SerialType>;
			};

			/// <summary>
			/// Convenience alias to obtain the direct serial output type for a given ProfilerLevelEnum.
			/// </summary>
			template<ProfilerLevelEnum Level, typename SerialType>
			using TemplateDirectSerialOutput = typename DirectSerialOutputSelector<Level, SerialType>::Type;
		}
	}
}
#endif