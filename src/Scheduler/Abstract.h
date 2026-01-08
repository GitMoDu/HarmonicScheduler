#ifndef _HARMONIC_SCHEDULER_ABSTRACT_h
#define _HARMONIC_SCHEDULER_ABSTRACT_h

#include "../Model/TaskRegistry.h"
#include "../Model/Profiling.h"
#include "../Platform/Atomic.h"

namespace Harmonic
{
	/// <summary>
	/// AbstractScheduler is the base class for statically-sized cooperative task runners.
	///
	/// - Inherits dynamic registration, removal, and management features from TaskRegistry.
	/// - Supports up to MaxTaskCount tasks (compile-time constant, enforced by static_assert).
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low power idle sleep when no tasks are ready.</typeparam>
	template<task_id_t MaxTaskCount>
	class AbstractScheduler : public TaskRegistry
	{
		static_assert(MaxTaskCount <= TASK_MAX_COUNT, "MaxTaskCount exceeds platform maximum task count (TASK_MAX_COUNT)");

	protected:
		/// <summary>
		/// Statically allocated array of TaskTracker objects, each representing a registered task.
		/// </summary>
		Platform::TaskTracker Tasks[MaxTaskCount]{};

	public:
		AbstractScheduler(const bool hotRegistry = false) : TaskRegistry(Tasks, MaxTaskCount, hotRegistry) {}

		/// <summary>
		/// Returns the time in milliseconds until the next scheduled task is due to run.
		/// </summary>
		/// <returns>Time in milliseconds until the next task is due.</returns>
		uint32_t GetTimeUntilNextRun() const
		{
			return GetTimeUntilNextRun<0>(Platform::GetTimestamp());
		}

		/// <summary>
		/// Advances the scheduler's notion of time, compensating for time spent in deep sleep.
		/// Rolls back the last execution time of all tasks by the specified offset.
		/// </summary>
		/// <param name="offset">Forward offset in milliseconds.</param>
		void AdvanceTimestamp(const uint32_t offset)
		{
			// Instead of adding a constant offset to the timestamp source (adding runtime overhead), 
			// the last execution time of all tasks is rolled back.
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				Tasks[i].LastRun -= offset;
			}
		}

	protected:
		/// <summary>
		/// Returns the shortest time in milliseconds until any task is due to run.
		/// If a task is due within 'shortest' ms, exits early for efficiency.
		/// </summary>
		/// <typeparam name="shortest">Early exit threshold in milliseconds.</typeparam>
		/// <param name="timestamp">Current timestamp.</param>
		/// <returns>Shortest time in milliseconds until the next task is due.</returns>
		template<uint32_t shortest = 0>
		uint32_t GetTimeUntilNextRun(const uint32_t timestamp) const
		{
			uint32_t shortestTime = UINT32_MAX;
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				const uint32_t timeUntilNext = Tasks[i].TimeUntilNextRun(timestamp);
				if (timeUntilNext < shortestTime)
				{
					shortestTime = timeUntilNext;
					if (shortestTime <= shortest)
					{
						break;
					}
				}
			}

			return shortestTime;
		}

		void IdleSleep()
		{
			// Only sleep when nothing was ran in this timestamp 
			// and is not set to run until the next millisecond or later.
#ifdef HARMONIC_PLATFORM_OS
			const uint32_t sleepDuration = GetTimeUntilNextRun<1>(Platform::GetTimestamp());
			if (sleepDuration > 1)
			{
				Platform::IdleSleep(IdleSleepSemaphore, sleepDuration);
			}
#else
				// Only sleep if no tasks are due immediately.
			const uint32_t timestamp = Platform::GetTimestamp();
			if (GetTimeUntilNextRun<0>(timestamp) != 0 // No tasks due immediately.
				&& !Hot // Not flagged hot by task interrupts.
				&& timestamp == Platform::GetTimestamp()) // No time advanced since last check.
			{
				Platform::IdleSleep(); // Safely micro-sleep until the next ms tick or interrupt.
			}
#endif
		}
	};

}
#endif