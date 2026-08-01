#ifndef _HARMONIC_PROFILING_TIMELINE_BUFFERED_SERIAL_OUTPUT_TASK_h
#define _HARMONIC_PROFILING_TIMELINE_BUFFERED_SERIAL_OUTPUT_TASK_h

#include "../Logging.h"
#include "../../Model/Profiling.h"
#include "../../Task/AbstractTask.h"

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
			class SystemLevelBufferedSerialOutputTask : public ISystemLevelListener, public AbstractTask
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

			public:
				SystemLevelBufferedSerialOutputTask(TaskRegistry& registry, ISystemLevelProfiler& profiler, SerialType& output)
					: ISystemLevelListener()
					, AbstractTask(registry)
					, Profiler(profiler)
					, Output(output)
				{}

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
						SetEnabled(false);
					}
				}

				virtual void OnTimelineResult(const SystemTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						SampleBuffer.Push(samples[i]);
					}

					SetEnabled(true);
				}

				bool Setup()
				{
					return Attach(0, false);
				}

				/// <summary>
				/// Starts the buffered serial output task.
				/// </summary>
				/// <param name="nameProvider">UNUSED task name provider. Only here to match the interface.</param>
				/// <returns>True if the task started successfully, false otherwise.</returns>
				bool Start(ITaskNameProvider* /*taskNameProvider*/ = nullptr)
				{
					if (Attach(0, true))
					{
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
					Detach();
				}
			};

			template<typename SerialType, size_t BufferSize = 64, uint32_t MaxOutputDuration = 500>
			class TaskLevelBufferedSerialOutputTask : public ITaskLevelListener, public AbstractTask
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

			public:
				TaskLevelBufferedSerialOutputTask(TaskRegistry& registry, ITaskLevelProfiler& profiler, SerialType& output)
					: ITaskLevelListener()
					, AbstractTask(registry)
					, Profiler(profiler)
					, Output(output)
				{}

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
						SetEnabled(false);
					}
				}

				virtual void OnTimelineResult(const TaskTimelineSample* samples, size_t sampleCount) override
				{
					for (size_t i = 0; i < sampleCount; i++)
					{
						SampleBuffer.Push(samples[i]);
					}

					SetEnabled(true);
				}

				bool Setup()
				{
					return Attach(0, false);
				}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					if (Attach(0, false))
					{
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
					Detach();
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