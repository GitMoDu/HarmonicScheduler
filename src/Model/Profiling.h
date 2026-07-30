#ifndef _HARMONIC_SCHEDULER_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_h

#include "../Platform/Platform.h"

namespace Harmonic
{
	enum class ProfilerModeEnum : uint8_t
	{
		// No profiling instrumentation or trace collection.
		None = 0,

		// Poll mode: Accumulates metrics in memory. The caller requests a snapshot
		// with RequestMetrics(), which is delivered through the listener callback.
		Metrics = 1,

		// Stream mode: Collects time-series trace samples in a buffer and delivers
		// them through the listener callback when the buffer reaches capacity.
		Timeline = 2
	};

	enum class ProfilerLevelEnum : uint8_t
	{
		// Metrics restricted to core scheduler.
		System = 0,

		// Detailed metrics or event tracing for individual tasks.
		Task = 1,
	};

	namespace Profiling
	{
		struct SystemMetrics
		{
			uint32_t Scheduling = 0;
			uint32_t Busy = 0;
			uint32_t IdleSleep = 0;

			bool HasData() const
			{
				return Scheduling > 0 || Busy > 0 || IdleSleep > 0;
			}
		};

		struct TaskMetrics
		{
			uint32_t Iterations = 0;
			uint32_t Duration = 0;
			uint32_t MaxDuration = 0;
			task_handle_t Handle = TASK_INVALID_HANDLE;
		};

		struct SystemTimelineSample
		{
			uint32_t Timestamp;
			task_handle_t Handle;
		};

		struct TaskTimelineSample
		{
			uint32_t Timestamp;
			task_handle_t Handle;
		};

		/// <summary>
		/// Interface for providing task names for known task handles. Used for profiling logging/output.
		/// </summary>
		struct ITaskNameProvider
		{
			/// <summary>
			/// Determines if a task handle is known.
			/// </summary>
			/// <param name="handle">The handle of the task.</param>
			/// <returns>True if the task handle is known; otherwise, false.</returns>
			virtual bool IsTaskKnown(const task_handle_t handle) const = 0;

			/// <summary>
			/// Retrieves the name of a known task given its handle.
			/// </summary>
			/// <param name="handle">The handle of the known task.</param>
			/// <returns>The name of the task.</returns>
			virtual const char* GetTaskName(const task_handle_t handle) const = 0;
		};

		/// <summary>
		/// Metrics snapshots are requested with RequestMetrics() and delivered to the
		/// listener at the end of a scheduler loop iteration.
		/// </summary>
		namespace Metrics
		{
			struct SystemSample
			{
				uint32_t Iterations = 0;
				uint32_t Scheduling = 0;
				uint32_t Busy = 0;
				uint32_t IdleSleep = 0;
			};

			struct TaskSample
			{
				uint32_t Iterations = 0;
				uint32_t Duration = 0;
				uint32_t MaxDuration = 0;
				task_handle_t Handle = TASK_INVALID_HANDLE;
			};

			/// <summary>
			/// Interface for receiving system-level profiling metric snapshots.
			/// A requested snapshot is delivered at the end of a scheduler loop iteration.
			/// </summary>
			struct ISystemLevelListener
			{
				ISystemLevelListener() = default;
				virtual ~ISystemLevelListener() = default;

				/// <summary>
				/// Called when a system-level profiling metrics result has been collected and is ready for retrieval.
				/// </summary>
				/// <param name="systemMetrics">The system-level profiling metrics result.</param>	
				virtual void OnMetricsResult(const SystemMetrics& systemMetrics) = 0;
			};

			/// <summary>
			/// Interface for receiving full task-level profiling metric snapshots.
			/// A requested snapshot is delivered at the end of a scheduler loop iteration.
			/// </summary>
			struct ITaskLevelListener
			{
				ITaskLevelListener() = default;
				virtual ~ITaskLevelListener() = default;

				/// <summary>
				/// Called when task-level profiling metrics results have been collected and are ready for retrieval.
				/// </summary>
				/// <param name="systemMetrics">Collected system-level metrics.</param>	
				/// <param name="tasksMetrics">Array of collected per-task metrics.</param>
				/// <param name="taskMetricsCount">Number of per-task metric items in the array.</param>
				virtual void OnMetricsResult(const SystemMetrics& systemMetrics, const TaskMetrics* tasksMetrics, const task_index_t taskMetricsCount) = 0;
			};

			/// <summary>
			/// Interface for a scheduler that supports system-level metrics collection.
			/// </summary>
			struct ISystemLevelProfiler
			{
				ISystemLevelProfiler() = default;
				virtual ~ISystemLevelProfiler() = default;

				/// <summary>
				/// Requests a system-level metrics snapshot from the scheduler.
				/// The result will be delivered asynchronously to the specified listener.
				/// </summary>
				/// <param name="resultListener">The listener to receive the metrics result.</param>
				/// <returns>True if the request was successfully initiated, false otherwise.</returns>
				virtual bool RequestMetrics(ISystemLevelListener* resultListener) = 0;

				/// <summary>
				/// Resets system-level metric accumulators.
				/// </summary>
				virtual void ResetMetrics() = 0;
			};

			/// <summary>
			/// Interface for a scheduler that supports task-level metrics collection.
			/// </summary>
			struct ITaskLevelProfiler
			{
				ITaskLevelProfiler() = default;
				virtual ~ITaskLevelProfiler() = default;

				/// <summary>
				/// Requests a full task-level metrics snapshot from the scheduler.
				/// The result will be delivered asynchronously to the specified listener.
				/// </summary>
				/// <param name="resultListener">The listener to receive the metrics result.</param>
				/// <returns>True if the request was successfully initiated, false otherwise.</returns>
				virtual bool RequestMetrics(ITaskLevelListener* resultListener) = 0;

				/// <summary>
				/// Resets all system and task-level metric accumulators.
				/// </summary>
				virtual void ResetMetrics() = 0;
			};
		}

		/// <summary>
		/// Timeline event samples are collected in a buffer during scheduler execution.
		/// When the buffer reaches capacity, its contents are delivered to the registered listener.
		/// </summary>
		namespace Timeline
		{
			/// <summary>
			/// Interface for receiving system-level timeline event streams.
			/// </summary>
			struct ISystemLevelListener
			{
				ISystemLevelListener() = default;
				virtual ~ISystemLevelListener() = default;

				/// <summary>
				/// Callback invoked when collected system timeline samples are ready.
				/// </summary>
				/// <param name="samples">Pointer to array of system timeline samples.</param>
				/// <param name="sampleCount">Number of samples in the array.</param>
				virtual void OnTimelineResult(const SystemTimelineSample* samples, const size_t sampleCount) = 0;
			};

			/// <summary>
			/// Interface for receiving task-level timeline event streams.
			/// </summary>
			struct ITaskLevelListener
			{
				ITaskLevelListener() = default;
				virtual ~ITaskLevelListener() = default;

				/// <summary>
				/// Callback invoked when collected task timeline samples are ready.
				/// </summary>
				/// <param name="samples">Pointer to array of task timeline samples.</param>
				/// <param name="sampleCount">Number of samples in the array.</param>
				virtual void OnTimelineResult(const TaskTimelineSample* samples, const size_t sampleCount) = 0;
			};

			/// <summary>
			/// Interface for setting up system-level timeline stream callbacks.
			/// </summary>
			struct ISystemLevelProfiler
			{
				ISystemLevelProfiler() = default;
				virtual ~ISystemLevelProfiler() = default;

				/// <summary>
				/// Registers or unregisters a listener for system-level timeline event streaming.
				/// </summary>
				/// <param name="resultListener">The listener to receive stream callbacks, or nullptr to unregister the current listener.</param>
				/// <returns>True if listener was updated successfully, false otherwise.</returns>
				virtual bool SetTimelineListener(ISystemLevelListener* resultListener) = 0;

				/// <summary>
				/// Clears the system-level timeline sample buffer.
				/// </summary>
				virtual void ResetTimeline() = 0;
			};

			/// <summary>
			/// Interface for setting up task-level timeline stream callbacks.
			/// </summary>
			struct ITaskLevelProfiler
			{
				ITaskLevelProfiler() = default;
				virtual ~ITaskLevelProfiler() = default;

				/// <summary>
				/// Registers or unregisters a listener for task-level timeline event streaming.
				/// </summary>
				/// <param name="resultListener">The listener to receive stream callbacks, or nullptr to unregister the current listener.</param>
				/// <returns>True if listener was updated successfully, false otherwise.</returns>
				virtual bool SetTimelineListener(ITaskLevelListener* resultListener) = 0;

				/// <summary>
				/// Clears the task-level timeline sample buffer.
				/// </summary>
				virtual void ResetTimeline() = 0;
			};
		}
	}
}
#endif