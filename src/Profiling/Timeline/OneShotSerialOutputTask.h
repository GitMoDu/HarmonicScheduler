#ifndef _HARMONIC_PROFILING_TIMELINE_ONE_SHOT_SERIAL_OUTPUT_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_ONE_SHOT_SERIAL_OUTPUT_TASK_h


#include "../Logging.h"
#include "../../Model/Profiling.h"
#include "../../Task/AbstractTask.h"


namespace Harmonic
{
	namespace Profiling
	{
		namespace Timeline
		{
			template<typename SerialType, size_t BufferSize = 1024>
			class SystemLevelOneShotSerialOutputTask : public ISystemLevelListener, public AbstractTask
			{
			private:
				SystemTimelineSample SampleBuffer[BufferSize]{};
				size_t SampleCount = 0;

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
				SystemLevelOneShotSerialOutputTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, SerialType& output)
					: ISystemLevelListener()
					, AbstractTask(registry)
					, Profiler(profiler)
					, Output(output)
				{}

				virtual void Run() override
				{
					SetEnabled(false);

					Logging::PrintTimelineSystemLevelHeader(Output);
					Logging::PrintTimelineStart(Output);

					for (size_t i = 0; i < SampleCount; i++)
					{
						Logging::PrintTimelineEntry(Output, SampleBuffer[i]);
					}
					SampleCount = 0;
				}

				virtual void OnTimelineResult(const SystemTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						if (SampleCount < BufferSize)
						{
							SampleBuffer[SampleCount++] = samples[i];
						}
						else
						{
							Profiler.SetTimelineListener(nullptr);
							SetEnabled(true);
							break;
						}
					}
				}

				bool Setup()
				{
					return Attach(0, false);
				}

				bool Start(ITaskNameProvider* /*nameProvider*/ = nullptr)
				{
					if (Attach(0, false)) {

						if (Profiler.SetTimelineListener(this))
						{
							return true;
						}
						else
						{
							Stop();
						}
					}

					return false;
				}

				void Stop()
				{
					Profiler.SetTimelineListener(nullptr);
					Detach();
				}
			};

			template<typename SerialType, size_t BufferSize = 1024>
			class TaskLevelOneShotSerialOutputTask :public ITaskLevelListener, public AbstractTask
			{
			private:
				TaskTimelineSample SampleBuffer[BufferSize]{};
				size_t SampleCount = 0;

			private:
				/// <summary>
				/// Profiler source reference.
				/// </summary>
				ITaskLevelProfiler& Profiler;

				/// <summary>
				/// Serial output reference.
				/// </summary>
				SerialType& Output;

			private:
				ITaskNameProvider* NameProvider = nullptr;

			public:
				TaskLevelOneShotSerialOutputTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, SerialType& output)
					: ITaskLevelListener()
					, AbstractTask(registry)
					, Profiler(profiler)
					, Output(output)
				{}

				task_handle_t GetHandle() const
				{
					return Handle;
				}

				virtual void Run() override
				{
					SetEnabled(false);

					Logging::PrintTimelineTaskLevelHeader(Output);
					Logging::PrintTimelineTaskNames(Output, NameProvider);
					Logging::PrintTimelineStart(Output);

					for (size_t i = 0; i < SampleCount; i++)
					{
						Logging::PrintTimelineEntry(Output, SampleBuffer[i]);
					}
					SampleCount = 0;
				}

				virtual void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						if (SampleCount < BufferSize)
						{
							SampleBuffer[SampleCount++] = samples[i];
						}
						else
						{
							break;
						}
					}

					if (SampleCount >= BufferSize)
					{
						Profiler.SetTimelineListener(nullptr);
						SetEnabled(true);
					}
				}

				bool Setup()
				{
					return Attach(0, false);
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Attach(0, false)) {

						if (Profiler.SetTimelineListener(this))
						{
							NameProvider = nameProvider;

							return true;
						}
						else
						{
							Stop();
						}
					}

					return false;
				}

				void Stop()
				{
					Profiler.SetTimelineListener(nullptr);
					Detach();
				}
			};

			/// <summary>
			/// Template selector that maps ProfilerLevelEnum to the appropriate buffered serial output task type.
			/// </summary>
			template<ProfilerLevelEnum Level, typename SerialType, size_t BufferSize>
			struct OneShotSerialOutputTaskSelector;

			template<typename SerialType, size_t BufferSize>
			struct OneShotSerialOutputTaskSelector<ProfilerLevelEnum::System, SerialType, BufferSize>
			{
				using Type = SystemLevelOneShotSerialOutputTask<SerialType, BufferSize>;
			};

			template<typename SerialType, size_t BufferSize>
			struct OneShotSerialOutputTaskSelector<ProfilerLevelEnum::Task, SerialType, BufferSize>
			{
				using Type = TaskLevelOneShotSerialOutputTask<SerialType, BufferSize>;
			};

			/// <summary>
			/// Convenience alias to obtain the one-shot serial output task type for a given ProfilerLevelEnum.
			/// </summary>
			template<ProfilerLevelEnum Level, typename SerialType, size_t BufferSize = 1024>
			using TemplateOneShotSerialOutputTask = typename OneShotSerialOutputTaskSelector<Level, SerialType, BufferSize>::Type;
		}
	}
}
#endif