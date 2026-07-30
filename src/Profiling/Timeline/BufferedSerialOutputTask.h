#ifndef _HARMONIC_PROFILING_TIMELINE_BUFFERED_SERIAL_OUTPUT_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_BUFFERED_SERIAL_OUTPUT_TASK_h

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
			namespace Detail
			{
				template<typename SampleType, size_t BufferSize>
				class TemplateSampleRingBuffer
				{
				private:
					static_assert(BufferSize > 0, "BufferSize must be greater than zero.");

					SampleType Buffer[BufferSize];
					size_t Head = 0;
					size_t Tail = 0;
					size_t Count = 0;

				public:
					TemplateSampleRingBuffer() {}

					bool IsFull() const
					{
						return Count == BufferSize;
					}

					bool IsEmpty() const
					{
						return Count == 0;
					}

					bool IsHalfFull() const
					{
						return Count >= (BufferSize / 2);
					}

					void Push(const SampleType& sample)
					{
						if (!IsFull())
						{
							Buffer[Head] = sample;
							Head = (Head + 1) % BufferSize;
							Count++;
						}
					}

					bool Pop(SampleType& sample)
					{
						if (!IsEmpty())
						{
							sample = Buffer[Tail];
							Tail = (Tail + 1) % BufferSize;
							Count--;
							return true;
						}
						return false;
					}
				};
			}

			template<typename SerialType, size_t BufferSize = 64, uint32_t MaxOutputDuration = 500>
			class SystemLevelBufferedSerialOutputTask : public ITask, public ISystemLevelListener
			{
			private:
				Detail::TemplateSampleRingBuffer<SystemTimelineSample, BufferSize> SampleBuffer{};

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
				SystemLevelBufferedSerialOutputTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, SerialType& output)
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
					const uint32_t startTime = Platform::GetProfilerTimestamp();
					SystemTimelineSample sample;
					while (SampleBuffer.Pop(sample))
					{
						Logging::PrintTimelineEntry(Output, sample);

						// Avoid blocking over the maximum output duration if the buffer is under half full.
						if (!SampleBuffer.IsHalfFull()
							&& ((Platform::GetProfilerTimestamp() - startTime) >= MaxOutputDuration))
						{
							break;
						}
					}

					if (SampleBuffer.IsEmpty())
					{
						Registry.SetEnabled(Handle, false);
					}
				}

				virtual void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						SampleBuffer.Push(samples[i]);
					}

					Registry.SetPeriodAndEnabled(Handle, 0, true);
				}

				bool Setup()
				{
					Handle = Registry.Attach(this, 0, false);
					return Handle != TASK_INVALID_HANDLE;
				}

				bool Start()
				{
					if (Handle == TASK_INVALID_HANDLE)
					{
						Handle = Registry.Attach(this, 0, false);
					}

					if (Handle != TASK_INVALID_HANDLE) {

						if (Profiler.SetTimelineListener(this))
						{
							Logging::PrintTimelineSystemLevelHeader(Output);
							Logging::PrintTimelineStart(Output);

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

			template<typename SerialType, size_t BufferSize = 64, uint32_t MaxOutputDuration = 500>
			class TaskLevelBufferedSerialOutputTask : public ITask, public ITaskLevelListener
			{
			private:
				Detail::TemplateSampleRingBuffer<TaskTimelineSample, BufferSize> SampleBuffer{};

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

			public:
				TaskLevelBufferedSerialOutputTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, SerialType& output)
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
					const uint32_t startTime = Platform::GetProfilerTimestamp();
					TaskTimelineSample sample;
					while (SampleBuffer.Pop(sample))
					{
						Logging::PrintTimelineEntry(Output, sample);

						// Avoid blocking over the maximum output duration if the buffer is under half full.
						if (!SampleBuffer.IsHalfFull()
							&& ((Platform::GetProfilerTimestamp() - startTime) >= MaxOutputDuration))
						{
							break;
						}
					}

					if (SampleBuffer.IsEmpty())
					{
						Registry.SetEnabled(Handle, false);
					}
				}

				virtual void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						SampleBuffer.Push(samples[i]);
					}

					Registry.SetPeriodAndEnabled(Handle, 0, true);
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
							Logging::PrintTimelineTaskLevelHeader(Output);
							Logging::PrintTimelineTaskNames(Output, nameProvider);
							Logging::PrintTimelineStart(Output);

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
			template<ProfilerLevelEnum Level, typename SerialType, size_t BufferSize = 64, uint32_t MaxOutputDuration = 500>
			struct BufferedSerialOutputTaskSelector;

			template<typename SerialType, size_t BufferSize, uint32_t MaxOutputDuration>
			struct BufferedSerialOutputTaskSelector<ProfilerLevelEnum::System, SerialType, BufferSize, MaxOutputDuration>
			{
				using Type = SystemLevelBufferedSerialOutputTask<SerialType, BufferSize, MaxOutputDuration>;
			};

			template<typename SerialType, size_t BufferSize, uint32_t MaxOutputDuration>
			struct BufferedSerialOutputTaskSelector<ProfilerLevelEnum::Task, SerialType, BufferSize, MaxOutputDuration>
			{
				using Type = TaskLevelBufferedSerialOutputTask<SerialType, BufferSize, MaxOutputDuration>;
			};

			/// <summary>
			/// Convenience alias to obtain the buffered serial output task type for a given ProfilerLevelEnum.
			/// </summary>
			template<ProfilerLevelEnum Level, typename SerialType, size_t BufferSize = 128, uint32_t MaxOutputDuration = 500>
			using TemplateBufferedSerialOutputTask = typename BufferedSerialOutputTaskSelector<Level, SerialType, BufferSize, MaxOutputDuration>::Type;
		}
	}
}
#endif