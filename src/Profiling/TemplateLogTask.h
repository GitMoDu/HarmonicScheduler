#ifndef _HARMONIC_PROFILING_TEMPLATE_LOG_TASK_h
#define _HARMONIC_PROFILING_TEMPLATE_LOG_TASK_h

#include "../Model/Profiling.h"
#include "../Model/TaskRegistry.h"

#include "Timeline/MetricsTaskLevelProfilerTask.h"
#include "Timeline/MetricsSystemLevelProfilerTask.h"
#include "Logging.h"

#include <Print.h>

namespace Harmonic
{
	namespace Profiling
	{
		/// <summary>
		/// Mock implementation of a log task for constexpr evaluation.
		/// Optimized out of existence when not used.
		/// </summary>
		/// <typeparam name="MaxTaskCount"></typeparam>
		/// <typeparam name="LogPeriod"></typeparam>
		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		class MockLogTask
		{
		public:
			MockLogTask(TaskRegistry& /*registry*/, TaskRegistry& /*mockProfiler*/, Print& /*output*/)
			{}

			bool Start(ITaskNameProvider* /*nameProvider*/ = nullptr)
			{
				return true;
			}

			task_handle_t GetTaskHandle() const
			{
				return TASK_INVALID_HANDLE;
			}

			void Stop()
			{}
		};

		/// <summary>
		/// Template selector that maps ProfilerModeEnum and ProfilerLevelEnum to the appropriate log task type.
		/// </summary>
		template<uint8_t MaxTaskCount, ProfilerModeEnum Mode, ProfilerLevelEnum Level, uint32_t LogPeriod>
		struct TemplateLogTaskSelector;

		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		struct TemplateLogTaskSelector<MaxTaskCount, ProfilerModeEnum::None, ProfilerLevelEnum::System, LogPeriod>
		{
			using Type = MockLogTask<MaxTaskCount, LogPeriod>;
		};

		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		struct TemplateLogTaskSelector<MaxTaskCount, ProfilerModeEnum::Metrics, ProfilerLevelEnum::System, LogPeriod>
		{
			using Type = Metrics::SystemLevelLogTask<MaxTaskCount, LogPeriod>;
		};

		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		struct TemplateLogTaskSelector<MaxTaskCount, ProfilerModeEnum::Metrics, ProfilerLevelEnum::Task, LogPeriod>
		{
			using Type = Metrics::TaskLevelLogTask<MaxTaskCount, LogPeriod>;
		};

		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		struct TemplateLogTaskSelector<MaxTaskCount, ProfilerModeEnum::Timeline, ProfilerLevelEnum::System, LogPeriod>
		{
			/// <summary>
			/// Combines the timeline system-level profiler with a system-level log task,
			/// to provide a complete logging solution for timeline traces.
			/// Serves as fallback for templated task logging when the profiling mode is set to Timeline.
			/// </summary>
			class Type : public Timeline::MetricsSystemLevelProfilerTask<MaxTaskCount>
			{
			private:
				using Base = Timeline::MetricsSystemLevelProfilerTask<MaxTaskCount>;

			private:
				Metrics::SystemLevelLogTask<MaxTaskCount, LogPeriod> AggregateLog;

			public:
				Type(TaskRegistry& registry, ::Harmonic::Profiling::Timeline::ISystemLevelProfiler& profiler, Print& output)
					: Base(registry, profiler)
					, AggregateLog(registry, *this, output)
				{}

				bool Start(ITaskNameProvider* /*nameProvider*/ = nullptr)
				{
					return Base::Start() && AggregateLog.Start();
				}

				void Stop()
				{
					AggregateLog.Stop();
					Base::Stop();
				}

				task_handle_t GetLogTaskHandle() const
				{
					return AggregateLog.GetTaskHandle();
				}
			};
		};

		template<uint8_t MaxTaskCount, uint32_t LogPeriod>
		struct TemplateLogTaskSelector<MaxTaskCount, ProfilerModeEnum::Timeline, ProfilerLevelEnum::Task, LogPeriod>
		{
			/// <summary>
			/// Combines the timeline task-level profiler with a full task-level log task,
			/// to provide a complete logging solution for timeline traces.
			/// Serves as fallback for templated task logging when the profiling mode is set to Timeline.
			/// </summary>
			class Type : public Timeline::MetricsTaskLevelProfilerTask<MaxTaskCount>
			{
			private:
				using Base = Timeline::MetricsTaskLevelProfilerTask<MaxTaskCount>;

			private:
				Metrics::TaskLevelLogTask<MaxTaskCount, LogPeriod> AggregateLog;

			public:
				Type(TaskRegistry& registry, ::Harmonic::Profiling::Timeline::ITaskLevelProfiler& profiler, Print& output)
					: Base(registry, profiler)
					, AggregateLog(registry, *this, output)
				{}

				bool Start(ITaskNameProvider* nameProvider = nullptr)
				{
					return Base::Start() && AggregateLog.Start(nameProvider);
				}

				void Stop()
				{
					AggregateLog.Stop();
					Base::Stop();
				}

				task_handle_t GetLogTaskHandle() const
				{
					return AggregateLog.GetTaskHandle();
				}
			};
		};

		/// <summary>
		/// Convenience alias to obtain the log task type for a given Profile mode and level.
		/// Example:
		///   using ProfileLogTaskType = TemplateLogTask<MaxTaskCount, ProfilerModeEnum::Timeline, ProfilerLevelEnum::Task, LogPeriod>;
		/// </summary>
		/// <typeparam name="MaxTaskCount">Maximum number of tasks supported by the scheduler.</typeparam>
		/// <typeparam name="Mode">Profiler mode enum value.</typeparam>
		/// <typeparam name="Level">Profiler level enum value.</typeparam>
		/// <typeparam name="LogPeriod">Log period in milliseconds.</typeparam>
		template<uint8_t MaxTaskCount, ProfilerModeEnum Mode, ProfilerLevelEnum Level, uint32_t LogPeriod>
		using TemplateLogTask = typename TemplateLogTaskSelector<MaxTaskCount, Mode, Level, LogPeriod>::Type;
	}
}
#endif