#ifndef _HARMONIC_PERIODIC_TASK_h
#define _HARMONIC_PERIODIC_TASK_h

#include "AbstractTask.h"

namespace Harmonic
{
	/// <summary>
	/// Base class for tasks that run periodically and can be dynamically registered and managed by a TaskRegistry.
	/// - Maintains a period and absolute next-due timestamp to schedule the next execution.
	/// - First run can be immediate or delayed by one period at Start().
	/// - Configurable scheduling mode: Reanchor (next run relative to last execution start) or PhaseLock (next run anchored to fixed grid).
	/// - Intended to be subclassed; override PeriodicRun() to implement task logic.
	///
	/// Below are examples of how the scheduling modes behave under different conditions:
	///
	/// IDEAL
	/// Ticks     |---------|---------|---------|---------|--->
	/// PhaseLock [=]       [=]       [=]       [=]
	/// Reanchor  [=]       [=]       [=]       [=]
	///
	/// EXTERNAL LATE (NO OVERRUN)
	/// Ticks     |---------|---------|---------|---------|--->
	/// PhaseLock [=]           [=]   [=]       [=]
	/// Reanchor  [=]           [=]       [=]       [=]
	///
	/// EXTERNAL VERY LATE (NO OVERRUN)
	/// Ticks     |---------|---------|---------|---------|--->
	/// PhaseLock [=]                    [=]	[=]
	/// Reanchor  [=]                    [=]       [=]
	///
	/// INTERNAL OVERRUN
	/// Ticks     |---------|---------|---------|---------|--->
	/// PhaseLock [=]       [============]      [=]
	/// Reanchor  [=]       [============][=]       [=]
	///
	/// EXTERNAL LATE + INTERNAL OVERRUN
	/// Ticks     |---------|---------|---------|---------|--->
	/// PhaseLock [=]          [============]   [=]
	/// Reanchor  [=]          [============][=]       [=]
	/// </summary>
	class PeriodicTask : public AbstractTask
	{
	public:
		enum class ScheduleModeEnum : uint8_t
		{
			Reanchor,   // Relative delay enforced from execution start (Tn + Period)
			PhaseLock // Anchored to a fixed absolute grid (T0 + N * Period)
		};

	private:
		uint32_t Period;
		uint32_t NextDue = 0;

	private:
		const ScheduleModeEnum ScheduleMode;

	protected:
		virtual void PeriodicRun() = 0;

	public:
		PeriodicTask(TaskRegistry& registry, const uint32_t period = 0, const ScheduleModeEnum scheduleMode = ScheduleModeEnum::Reanchor)
			: AbstractTask(registry)
			, Period(period)
			, ScheduleMode(scheduleMode)
		{}

		bool Start(bool immediate = true)
		{
			if (Period == 0)
			{
				// Period of 0 is invalid for a periodic task. Use a non-zero period.
				return false;
			}

			return Start(Period, immediate);
		}

		/// <summary>
		/// Starts the periodic task with the specified period.
		/// Attaches the task to the registry if not already attached.
		/// Based on the 'immediate' parameter, the first execution can occur immediately or after the first period.
		/// </summary>
		/// <param name="period">The period of the task in milliseconds. Must be greater than 0.</param>
		/// <param name="immediate">If true, the task will run immediately; otherwise, it will run after the first period.</param>
		/// <returns>True if the task was successfully started, false otherwise.</returns>
		bool Start(const uint32_t period, bool immediate = true)
		{
			if (period == 0)
			{
				// Period of 0 is invalid for a periodic task. Use a non-zero period.
				return false;
			}

			const uint32_t now = Platform::GetTimestamp();

			if (!Attach(0, false))
			{
				return false;
			}

			Period = period;
			NextDue = immediate ? now : (now + Period);
			Registry.SetDelay(AbstractTask::Handle, immediate ? 0u : Period);
			SetEnabled(true);

			return true;
		}

		/// <summary>
		/// Stops the periodic task. 
		/// Detaches the task from the registry and prevents further executions.
		/// </summary>
		void Stop()
		{
			Detach();
		}

		/// <summary>
		/// Gets the period of the periodic task in milliseconds.
		/// </summary>
		/// <returns>The period of the task in milliseconds.</returns>
		uint32_t GetPeriod() const
		{
			return Period;
		}

		/// <summary>
		/// Synchronizes the next due time to the current timestamp.
		/// Period for the next run will be calculated from the current time, effectively resetting the schedule.
		/// </summary>
		void SyncToNow()
		{
			NextDue = Platform::GetTimestamp();
			Registry.SetDelayFromNow(AbstractTask::Handle, 0);
		}

		/// <summary>
		/// Gets the overrun mode of the periodic task.
		/// </summary>
		/// <returns>The schedule mode of the task.</returns>
		ScheduleModeEnum GetScheduleMode() const
		{
			return ScheduleMode;
		}

		void Run() override
		{
			// Capture the start timestamp to calculate the next due time based on the actual execution time.
			const uint32_t runStart = Platform::GetTimestamp();

			// Run the periodic task logic implemented in the derived class.
			PeriodicRun();

			// If the task is detached early exit.
			if (GetTaskHandle() == TASK_INVALID_HANDLE)
			{
				return;
			}
			else
			{
				// Calculate the next due time based on the scheduling mode.
				const uint32_t now = Platform::GetTimestamp();

				if (ScheduleMode == ScheduleModeEnum::PhaseLock)
				{
					NextDue += Period;
					if (static_cast<int32_t>(now - NextDue) >= 0)
					{
						NextDue += (static_cast<uint32_t>(now - NextDue) / Period + 1u) * Period;
					}
				}
				else
				{
					NextDue = runStart + Period;
				}

				// Set the delay for the next execution.
				const uint32_t delay = static_cast<int32_t>(NextDue - now) > 0 ? NextDue - now : 0;
				Registry.SetDelayFromNow(AbstractTask::Handle, delay);
			}
		}
	};
}
#endif