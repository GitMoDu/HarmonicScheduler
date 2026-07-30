#ifndef _HARMONIC_TEMPLATE_SCHEDULER_h
#define _HARMONIC_TEMPLATE_SCHEDULER_h

#include "AbstractScheduler.h"
#include "NoProfiling.h"
#include "Metrics/SystemLevelProfiling.h"
#include "Metrics/TaskLevelProfiling.h"
#include "Timeline/SystemLevelProfiling.h"
#include "Timeline/TaskLevelProfiling.h"

namespace Harmonic
{
	namespace Selector
	{
		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, ProfilerModeEnum Mode, ProfilerLevelEnum Level, size_t TraceSampleCount>
		struct TemplateSchedulerSelector;

		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::None, ProfilerLevelEnum::System, TraceSampleCount>
		{
			using Type = SchedulerNoProfiling<MaxTaskCount, IdleSleepEnabled>;
		};
		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::None, ProfilerLevelEnum::Task, TraceSampleCount>
		{
			using Type = SchedulerNoProfiling<MaxTaskCount, IdleSleepEnabled>;
		};

		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::Metrics, ProfilerLevelEnum::System, TraceSampleCount>
		{
			using Type = Profiling::Metrics::SystemLevelProfiling<MaxTaskCount, IdleSleepEnabled>;
		};

		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::Metrics, ProfilerLevelEnum::Task, TraceSampleCount>
		{
			using Type = Profiling::Metrics::TaskLevelProfiling<MaxTaskCount, IdleSleepEnabled>;
		};

		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::Timeline, ProfilerLevelEnum::System, TraceSampleCount>
		{
			using Type = Profiling::Timeline::SystemLevelProfiling<MaxTaskCount, IdleSleepEnabled, TraceSampleCount>;
		};

		template<task_index_t MaxTaskCount, bool IdleSleepEnabled, size_t TraceSampleCount>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfilerModeEnum::Timeline, ProfilerLevelEnum::Task, TraceSampleCount>
		{
			using Type = Profiling::Timeline::TaskLevelProfiling<MaxTaskCount, IdleSleepEnabled, TraceSampleCount>;
		};
	}

	/// <summary>
	/// Templated scheduler selector based on the profiling mode, level and idle sleep settings.
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported by the scheduler.</typeparam>
	/// <typeparam name="Mode">Profiler mode enum value.</typeparam>
	/// <typeparam name="Level">Profiler level enum value.</typeparam>
	/// <typeparam name="IdleSleepEnabled">Indicates whether idle sleep is enabled.</typeparam>
	/// <typeparam name="TraceSampleCount">Number of trace samples for timeline profiling. Unused for other profiling modes.</typeparam>
	template<task_index_t MaxTaskCount, bool IdleSleepEnabled = false, ProfilerModeEnum Mode = ProfilerModeEnum::None, ProfilerLevelEnum Level = ProfilerLevelEnum::System, size_t TraceSampleCount = 32>
	using TemplateScheduler = typename Selector::TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, Mode, Level, TraceSampleCount>::Type;
}
#endif