#ifndef _HARMONIC_SCHEDULER_PROFILING_TIMELINE_TASK_LEVEL_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_TIMELINE_TASK_LEVEL_PROFILING_h

#include "../AbstractScheduler.h"

namespace Harmonic
{
	namespace Profiling
	{
		namespace Timeline
		{
			template<task_index_t MaxTaskCount, bool IdleSleepEnabled = false, size_t TimelineSampleCount = 100>
			class TaskLevelProfiling
				: public ITaskLevelProfiler
				, public AbstractScheduler<MaxTaskCount>
			{
			public:
				static constexpr size_t MaxIterationSampleCount = (static_cast<size_t>(MaxTaskCount) * 2U)
					+ (IdleSleepEnabled ? 2U : 0U);

			private:
				using Base = AbstractScheduler<MaxTaskCount>;
				using IdleSleepTag = typename ConditionalDispatch::conditional_type<IdleSleepEnabled>::type;

				static_assert(MaxTaskCount <= (TASK_MAX_COUNT - 1), "MaxTaskCount exceeds maximum task count (TASK_MAX_COUNT - 1)");
				static_assert(TimelineSampleCount >= (MaxIterationSampleCount + 1U), "TimelineSampleCount is too small for a complete scheduler iteration");

			private:
				TaskTimelineSample Traces[TimelineSampleCount]{};
				size_t TraceCount = 0;
				ITaskLevelListener* ResultListener = nullptr;

			protected:
				using Base::Tasks;
				using Base::TaskCount;
				using Base::Hot;
				using Base::IdleSleep;

			public:
				TaskLevelProfiling()
					: ITaskLevelProfiler()
					, Base(IdleSleepEnabled)
				{}

				/// <summary>
				/// Requests a timeline profiling trace result from the scheduler.
				/// The result will be delivered asynchronously to the specified listener.
				/// </summary>
				/// <param name="resultListener">The listener to receive the timeline trace result.</param>
				/// <returns>True if the request was successfully initiated, false otherwise.</returns>
				bool SetTimelineListener(ITaskLevelListener* resultListener) override
				{
					ResultListener = resultListener;
					return ResultListener != nullptr;
				}

				void ClearTraceData()
				{
					TraceCount = 0;
				}

				virtual void ResetTimeline() override
				{
					ClearTraceData();
				}

				/// <summary>
				/// Runs one scheduler iteration without collecting profiling statistics.
				/// </summary>
				void Loop()
				{
					Loop(IdleSleepTag{});
				}

			private:
				void Loop(ConditionalDispatch::TrueType)
				{
					if (TraceCount > 0
						&& (TraceCount + MaxIterationSampleCount) >= (TimelineSampleCount / 1))
					{
						// Notify before starting an iteration that cannot fit in the remaining trace buffer.
						const uint32_t traceStart = Platform::GetProfilerTimestamp();
						if (ResultListener != nullptr)
						{
							ResultListener->OnTimelineResult(Traces, TraceCount);
						}
						ClearTraceData();
						AddTrace(traceStart, TRACE_TASK_HANDLE);
						AddTrace(TRACE_SCHEDULER_HANDLE);
					}

					Hot = false;
					for (task_index_t i = 0; i < TaskCount; i++)
					{
						const uint32_t timestamp = Platform::GetTimestamp();
						if (Tasks[i].ShouldRun(timestamp))
						{
							// Wrap the task execution with timeline samples to record the start and end of the task execution.
							AddTrace(Tasks[i].Handle);
							Tasks[i].RunDirect(timestamp);
							AddTrace(TRACE_SCHEDULER_HANDLE);
							Hot = true;
						}
					}

					if (!Hot)
					{
						// Wrap the idle sleep with timeline samples to record the start and end of the idle sleep period.
						AddTrace(TRACE_SLEEP_HANDLE);
						IdleSleep();
						AddTrace(TRACE_SCHEDULER_HANDLE);
					}
				}

				void Loop(ConditionalDispatch::FalseType)
				{
					if (TraceCount > 0
						&& (TraceCount + MaxIterationSampleCount) >= (TimelineSampleCount / 1))
					{
						// Notify before starting an iteration that cannot fit in the remaining trace buffer.
						const uint32_t traceStart = Platform::GetProfilerTimestamp();
						if (ResultListener != nullptr)
						{
							ResultListener->OnTimelineResult(Traces, TraceCount);
						}
						ClearTraceData();
						AddTrace(traceStart, TRACE_TASK_HANDLE);
						AddTrace(TRACE_SCHEDULER_HANDLE);
					}

					for (task_index_t i = 0; i < TaskCount; i++)
					{
						const uint32_t timestamp = Platform::GetTimestamp();
						if (Tasks[i].ShouldRun(timestamp))
						{
							// Wrap the task execution with timeline samples to record the start and end of the task execution.
							AddTrace(Tasks[i].Handle);
							Tasks[i].RunDirect(timestamp);
							AddTrace(TRACE_SCHEDULER_HANDLE);
						}
					}
				}

				void AddTrace(const uint32_t timestamp, const task_handle_t handle)
				{
					if (TraceCount < TimelineSampleCount)
					{
						Traces[TraceCount] = TaskTimelineSample{ timestamp, handle };
						TraceCount++;
					}
				}

				void AddTrace(const task_handle_t handle)
				{
					AddTrace(Platform::GetProfilerTimestamp(), handle);
				}
			};
		}
	}

	// Alias for the task-level timeline profiling scheduler.
	template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
	using SchedulerTimelineTaskLevel = Profiling::Timeline::TaskLevelProfiling<MaxTaskCount, IdleSleepEnabled, TraceSampleCount>;
}
#endif