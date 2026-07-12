#ifndef _HARMONIC_SCHEDULER_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_h

#include <stdint.h>

#include "../Platform/Platform.h"

namespace Harmonic
{
	enum class ProfileLevelEnum : uint8_t
	{
		// No profiling instrumentation or trace collection.
		None = 0,
		// Aggregate scheduler timing without per-task statistics.
		Base = 1,
		// Aggregate timing plus per-task execution statistics.
		Full = 2
	};

	namespace Profiling
	{
		struct TaskTrace
		{
			task_handle_t Handle = TASK_INVALID_HANDLE;
			uint32_t Duration = 0;
			uint32_t MaxDuration = 0;
			uint32_t Iterations = 0;
		};

		struct BaseTrace
		{
			uint32_t Iterations;
			uint32_t Scheduling;
			uint32_t Busy;
			uint32_t IdleSleep;
		};

		struct FullTrace
		{
			uint32_t Iterations;
			uint32_t Scheduling;
			uint32_t IdleSleep;
			uint8_t TaskCount;
		};

		/// <summary>
		/// Interface for receiving base profiling trace results asynchronously.
		/// Results are delivered at the end of a scheduler loop iteration when RequestTrace() is called, to ensure data consistency.
		/// </summary>
		struct IBaseProfilerListener
		{
			IBaseProfilerListener() = default;
			virtual ~IBaseProfilerListener() = default;

			/// <summary>
			/// Called when a base profiling trace result has been collected and is ready for retrieval.
			/// </summary>
			/// <param name="trace">The base profiling trace result.</param>
			virtual void OnTraceResult(const Profiling::BaseTrace& trace) = 0;
		};

		/// <summary>
		/// Interface for a scheduler that supports base profiling and trace retrieval.
		/// </summary>
		struct IBaseProfiler
		{
			IBaseProfiler() = default;
			virtual ~IBaseProfiler() = default;

			/// <summary>
			/// Requests a base profiling trace result from the scheduler.
			/// The result will be delivered asynchronously to the specified listener.
			/// </summary>
			/// <param name="resultListener">The listener to receive the trace result.</param>
			/// <returns>True if the request was successfully initiated, false otherwise.</returns>
			virtual bool RequestTrace(IBaseProfilerListener* resultListener) = 0;

			/// <summary>
			/// Resets the base profiling trace data.
			/// </summary>
			virtual void ResetTrace() = 0;
		};

		/// <summary>
		/// Interface for receiving full profiling trace results asynchronously.
		/// Results are delivered at the end of a scheduler loop iteration when RequestTrace() is called, to ensure data consistency.
		/// </summary>
		struct IFullProfilerListener
		{
			IFullProfilerListener() = default;
			virtual ~IFullProfilerListener() = default;

			/// <summary>
			/// Called when a full profiling trace result has been collected and is ready for retrieval.
			/// </summary>
			/// <param name="trace">The full profiling trace result.</param>
			/// <param name="taskTraces">An array of task-specific trace results.</param>
			/// <param name="taskCount">The number of task-specific trace results.</param>
			virtual void OnTraceResult(const Profiling::FullTrace& trace, const Profiling::TaskTrace* taskTraces, const uint8_t taskCount) = 0;
		};

		struct IFullProfiler
		{
			IFullProfiler() = default;
			virtual ~IFullProfiler() = default;

			/// <summary>
			/// Requests a full profiling trace result from the scheduler.
			/// The result will be delivered asynchronously to the specified listener.
			/// </summary>
			/// <param name="resultListener">The listener to receive the trace result.</param>
			/// <returns>True if the request was successfully initiated, false otherwise.</returns>
			virtual bool RequestTrace(IFullProfilerListener* resultListener) = 0;

			/// <summary>
			/// Resets the full profiling trace data, including both global and per-task statistics.
			/// </summary>
			virtual void ResetTrace() = 0;
		};
	}
}
#endif