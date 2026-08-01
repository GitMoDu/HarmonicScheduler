#ifndef _HARMONIC_PERIODIC_TASK_h
#define _HARMONIC_PERIODIC_TASK_h

#include "AbstractTask.h"

namespace Harmonic
{
	/// <summary>
	/// Base class for tasks that run periodically and can be dynamically registered and managed by a TaskRegistry.
	/// - Maintains a period and absolute next-due timestamp to schedule the next execution.
	/// - First run can be immediate or delayed by one period at Start().
	/// - Configurable overrun behavior: Sync (phase-locked) or Resync (catch-up).
	/// - Intended to be subclassed; override PeriodicRun() to implement task logic.
	/// </summary>
	class PeriodicTask : public AbstractTask
	{
	public:
		enum class OverrunModeEnum
		{
			Sync, // V-Sync like behavior: if a run is delayed, the next run is scheduled to maintain the original phase.
			Resync // VRR like behavior: if a run is delayed more than 2 * Period, the next run is scheduled immediately to resync with the current time.
		};

	private:
		uint32_t Period = 0;
		uint32_t NextDue = 0;

	private:
		const OverrunModeEnum OverrunMode;

	protected:
		virtual void PeriodicRun() = 0;

	public:
		PeriodicTask(TaskRegistry& registry, const OverrunModeEnum overrunMode = OverrunModeEnum::Sync)
			: AbstractTask(registry), OverrunMode(overrunMode)
		{}

		/// <summary>
		/// Starts the periodic task with the specified period.
		/// Attaches the task to the registry if not already attached.
		/// Based on the 'immediate' parameter, the first execution can occur immediately or after the first period.
		/// </summary>
		/// <param name="period">The period of the task in milliseconds.</param>
		/// <param name="immediate">If true, the task will run immediately; otherwise, it will run after the first period.</param>
		/// <returns>True if the task was successfully started, false otherwise.</returns>
		bool Start(const uint32_t period, bool immediate = true)
		{
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

		OverrunModeEnum GetOverrunMode() const
		{
			return OverrunMode;
		}

		void Run() override
		{
			if (Period < 2)
			{
				// Timing resolution level periods, run immediately without scheduling adjustments.
				PeriodicRun();
			}
			else
			{
				const uint32_t runStart = Platform::GetTimestamp();

				PeriodicRun();

				if (GetTaskHandle() == TASK_INVALID_HANDLE || Period == 0)
				{
					return;
				}

				const uint32_t runEnd = Platform::GetTimestamp();
				const uint32_t lateness = runStart - NextDue;

				if (OverrunMode == OverrunModeEnum::Resync
					&& Period > 1
					&& ((lateness >> 1) > Period))
				{
					// Hard resync: run once ASAP and re-anchor phase at this run start.
					NextDue = runStart;
				}
				else
				{
					// Phase-locked progression: keep stepping schedule by one period.
					NextDue += Period + (lateness / Period);
				}

				const uint32_t wait = NextDue - runEnd;
				if (wait >= Period)
				{
					Registry.SetDelay(Handle, 0);
				}
				else
				{
					Registry.SetDelay(Handle, wait);
				}
			}
		}
	};
}
#endif