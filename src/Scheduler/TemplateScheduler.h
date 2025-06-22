#ifndef _HARMONIC_TEMPLATE_SCHEDULER_h
#define _HARMONIC_TEMPLATE_SCHEDULER_h

#include "../Model/ITask.h"
#include "TrackedTask.h"
#include "Platform.h"

namespace Harmonic
{
	/// <summary>
	/// Harmonic Cooperative Scheduler, implementation for IScheduler with templated task count.
	/// Millisecond accuracy task callback, as long as the total execution time for each loop does not exceed 1000us.
	/// Idle sleep feature implementation is automatically selected for each platform.
	/// </summary>
	/// <typeparam name="MaxTaskCount">[0 ; 255]</typeparam>
	/// <typeparam name="IdleSleepEnable">Enable low power idle sleep.</typeparam>
	template<const uint8_t MaxTaskCount,
		const bool IdleSleepEnable = false>
	class TemplateScheduler : public IScheduler
	{
	private:
		// Tracked tasks and counter.
		TrackedTask Tasks[MaxTaskCount]{};
		uint8_t TaskCount = 0;

		// Flag to track if scheduling changes during runtime or interrupts.
		volatile bool Hot = false;

	public:
		TemplateScheduler() : IScheduler()
		{
		}

		/// <summary>
		/// </summary>
		/// <returns>How many task are attached.</returns>
		virtual uint8_t GetTaskCount() const final
		{
			return TaskCount;
		}

		/// <summary>
		/// </summary>
		/// <returns>How many task are attached.</returns>
		virtual uint8_t GetMaxTaskCount() const final
		{
			return MaxTaskCount;
		}

		/// <summary>
		/// How long until the next schedulled task.
		/// </summary>
		/// <returns>Time in milliseconds.</returns>
		virtual uint32_t GetTimeUntilNextRun() const final
		{
			return GetTimeUntilNextRun(Platform::GetTimestamp());
		}

		void Loop()
		{
			const uint32_t timestamp = Platform::GetTimestamp();

			// Compile time constant, branch is picked during compilation, not runtime.
			if (IdleSleepEnable)
			{
				// Reset flag and loop all tasks.
				Hot = false;
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].TimeUntilNextRun(timestamp) == 0)
					{
						Tasks[i].Run(timestamp);
						Hot = true; // Flag loop as hot.
					}
				}

				if (Hot)
				{
					// If flag became hot, loop again before trying to sleep.
					Hot = false;
				}
				else
				{
					// Only sleep when nothing was ran in this timestamp 
					// and is not set to run in the next.
					const bool shouldSleep = GetTimeUntilNextRun(Platform::GetTimestamp()) > 1;
					if (shouldSleep && !Hot)
					{
						Platform::IdleSleep();
					}
				}
			}
			else
			{
				for (uint_fast8_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].TimeUntilNextRun(timestamp) == 0)
					{
						Tasks[i].Run(timestamp);
					}
				}
			}
		}

		/// <summary>
		/// Forward scheduler time, to compensate for deep sleep.
		/// </summary>
		/// <param name="offset">Forward offset in milliseconds.</param>
		virtual void AdvanceTimestamp(const uint32_t offset) final
		{
			// Instead of adding a constant offset to the timestamp source (adding runtime overhead), 
			// the last execution time of all tasks is rolled back.
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				Tasks[i].Rollback(offset);
			}
		}

	public:
		/// <summary>
		/// Attach a task to the scheduler at setup time.
		/// Once added, a task cannot be removed.
		/// Adding tasks after runtime has started is not supported.
		/// </summary>
		/// <param name="task">Pointer to Harmonic ITask interface.</param>
		/// <param name="delay">Task execution delay, default 0.</param>
		/// <param name="enabled">Task state, default enabled.</param>
		/// <returns>True on success.</returns>
		virtual bool Attach(ITask* task, task_id_t& taskId, const uint32_t delay = 0, const bool enabled = true) final
		{
			const uint32_t timestamp = Platform::GetTimestamp();

			if (task != nullptr
				&& TaskCount < MaxTaskCount
				&& !TaskExists(task))
			{
				taskId = TaskCount;
				Tasks[taskId].SetTask(task, timestamp, delay, enabled);
				TaskCount++;

				return true;
			}

			return false;
		}

		/// <summary>
		/// Set task execution delay period.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="delay">Task execution period.</param>
		virtual void SetDelay(const uint8_t taskId, const uint32_t delay) final
		{
			Tasks[taskId].SetDelay(delay);
			if (IdleSleepEnable)
				Hot = true;
		}

		/// <summary>
		/// Enable/Disable task from execution.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="enabled">Task state.</param>
		virtual void SetEnabled(const uint8_t taskId, const bool enabled)
		{
			Tasks[taskId].SetEnabled(enabled);
			if (IdleSleepEnable)
				Hot = true;
		}

		/// <summary>
		/// Set task execution period and Enable/Disable task from execution.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="delay">Task execution period.</param>
		/// <param name="enabled">Task state.</param>
		virtual void Set(const uint8_t taskId, const uint32_t delay, const bool enabled) final
		{
			Tasks[taskId].Set(delay, enabled);
			if (IdleSleepEnable)
				Hot = true;
		}

	private:
		/// <summary>
		/// Check all tasks for the shortest time to next execution.
		/// </summary>
		/// <param name="timestamp"></param>
		/// <returns></returns>
		uint32_t GetTimeUntilNextRun(const uint32_t timestamp) const
		{
			uint32_t shortestTime = UINT32_MAX;
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				const uint32_t timeUntilNext = Tasks[i].TimeUntilNextRun(timestamp);
				if (timeUntilNext < shortestTime)
				{
					shortestTime = timeUntilNext;
					if (shortestTime == 0)
					{
						break;
					}
				}
			}

			return shortestTime;
		}

		bool TaskExists(const Harmonic::ITask* task) const
		{
			for (uint_fast8_t i = 0; i < TaskCount; i++)
			{
				if (Tasks[i].GetTask() == task)
				{
					return true;
				}
			}

			return false;
		}
	};
}
#endif