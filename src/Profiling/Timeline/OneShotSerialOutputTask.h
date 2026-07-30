#ifndef _HARMONIC_PROFILING_TIMELINE_ONE_SHOT_SERIAL_OUTPUT_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_ONE_SHOT_SERIAL_OUTPUT_TASK_h


#include "../Logging.h"
#include "../../Model/Profiling.h"
#include "../../Model/ITask.h"
#include "../../Model/TaskRegistry.h"


namespace Harmonic
{
	namespace Profiling
	{
		namespace Timeline
		{
			template<typename SerialType, size_t BufferSize = 1024>
			class SystemLevelOneShotSerialOutputTask : public ITask, public ISystemLevelListener
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

			protected:
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

			public:
				SystemLevelOneShotSerialOutputTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, SerialType& output)
					: ITask()
					, ISystemLevelListener()
					, Profiler(profiler)
					, Output(output)
					, Registry(registry)
				{}

				task_handle_t GetHandle() const
				{
					return Handle;
				}

				virtual void Run() override
				{
					Registry.SetEnabled(Handle, false);

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
							Registry.SetEnabled(Handle, true);
							break;
						}
					}
				}

				bool Setup()
				{
					Handle = Registry.Attach(this, 0, false);
					return Handle != TASK_INVALID_HANDLE;
				}

				bool Start(ITaskNameProvider* /*nameProvider*/ = nullptr)
				{
					if (Handle == TASK_INVALID_HANDLE)
					{
						Handle = Registry.Attach(this, 0, false);
					}

					if (Handle != TASK_INVALID_HANDLE) {

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
					Registry.Detach(Handle);
					Handle = TASK_INVALID_HANDLE;
				}
			};

			template<typename SerialType, size_t BufferSize = 1024>
			class TaskLevelOneShotSerialOutputTask : public ITask, public ITaskLevelListener
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

			protected:
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

			public:
				TaskLevelOneShotSerialOutputTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, SerialType& output)
					: ITask()
					, ITaskLevelListener()
					, Profiler(profiler)
					, Output(output)
					, Registry(registry)
				{}

				task_handle_t GetHandle() const
				{
					return Handle;
				}

				virtual void Run() override
				{
					Registry.SetEnabled(Handle, false);

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
						Registry.SetEnabled(Handle, true);
					}
				}

				bool Setup()
				{
					Handle = Registry.Attach(this, 0, false);
					return Handle != TASK_INVALID_HANDLE;
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Handle == TASK_INVALID_HANDLE)
					{
						Handle = Registry.Attach(this, 0, false);
					}

					if (Handle != TASK_INVALID_HANDLE) {

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
					Registry.Detach(Handle);
					Handle = TASK_INVALID_HANDLE;
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