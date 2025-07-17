#ifndef _HARMONIC_TEMPLATE_SCHEDULER_h
#define _HARMONIC_TEMPLATE_SCHEDULER_h

#include "../Model/TaskRegistry.h"
#include "../Platform/Atomic.h"

namespace Harmonic
{
	/// <summary>
	/// TemplateScheduler is a statically-sized cooperative task runner.
	///
	/// - Inherits dynamic registration, removal, and management features from TaskRegistry.
	/// - Supports up to MaxTaskCount tasks (compile-time constant, enforced by static_assert).
	/// - Optionally enters low-power idle sleep when no tasks are scheduled to run (IdleSleepEnabled).
	/// - The Loop() method should be called frequently (e.g., from the main application loop).
	/// </summary>
	/// <typeparam name="MaxTaskCount">Maximum number of tasks supported (must not exceed TASK_MAX_COUNT).</typeparam>
	/// <typeparam name="IdleSleepEnabled">Enable low power idle sleep when no tasks are ready.</typeparam>
	template<task_id_t MaxTaskCount, bool IdleSleepEnabled = false>
	class TemplateScheduler : public TaskRegistry
	{
		static_assert(MaxTaskCount <= TASK_MAX_COUNT, "MaxTaskCount exceeds platform maximum task count (TASK_MAX_COUNT)");

	private:
		/// <summary>
		/// Statically allocated array of TaskTracker objects, each representing a registered task.
		/// </summary>
		Platform::TaskTracker Tasks[MaxTaskCount]{};

	public:
		TemplateScheduler() : TaskRegistry(Tasks, MaxTaskCount) {}

		/// <summary>
		/// Main scheduler loop.
		/// Iterates through all registered tasks and runs those that are due.
		/// If IdleSleepEnabled is true, enters low-power sleep when no tasks are scheduled to run.
		/// Should be called as often as possible (e.g., in the main application loop).
		/// </summary>
		void Loop()
		{
			if (IdleSleepEnabled)
			{
				// Reset flag and loop all tasks.
				Hot = false;
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].RunIfTime())
					{
						Hot = true; // Flag loop as hot.
					}
				}

				if (!Hot)
				{
					// If flag became hot, loop again before trying to sleep.
					// Only sleep when nothing was ran in this timestamp 
					// and is not set to run in the next millisecond.
#ifdef HARMONIC_PLATFORM_OS
					const uint32_t sleepDuration = GetTimeUntilNextRun<1>(Platform::GetTimestamp());
					if (sleepDuration > 1)
					{
						Platform::IdleSleep(IdleSleepSemaphore, sleepDuration);
					}
#else
					const bool shouldSleep = GetTimeUntilNextRun<1>(Platform::GetTimestamp()) > 1;
					if (shouldSleep && !Hot)
					{
						Platform::IdleSleep();
					}
#endif
				}
			}
			else
			{
				// Run all tasks that are due, without idle sleep.
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					Tasks[i].RunIfTime();
				}
			}
		}

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

	private:
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
	};
}
#endif