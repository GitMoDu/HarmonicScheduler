#ifndef _HARMONIC_TASK_TRACKER_h
#define _HARMONIC_TASK_TRACKER_h

#include "ITask.h"

namespace Harmonic
{
	namespace Platform
	{
		/// <summary>
		/// Tracks and manages the execution of a single ITask.
		/// </summary>
		struct TaskTracker
		{
			/// <summary>
			/// Pointer to the associated task to be managed.
			/// </summary>
			ITask* Task = nullptr;

			/// <summary>
			/// Minimum delay (in ms) between consecutive task runs.
			/// </summary>
			volatile uint32_t Delay = 0;

			/// <summary>
			/// Timestamp (in ms) of the last time the task was run.
			/// </summary>
			uint32_t LastRun = 0;

			/// <summary>
			/// Indicates whether the task is enabled and eligible to run.
			/// </summary>
			volatile bool Enabled = false;

			/// <summary>
			/// Runs the task if it is enabled and the delay has elapsed since the last run.
			/// Updates LastRun if the task is executed.
			/// 
			/// Reads of 'Enabled' and 'Delay' are performed atomically by disabling interrupts
			/// during the read. This prevents race conditions with ISRs that may modify these
			/// variables, ensuring a consistent snapshot of their values.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>True if the task was run, false otherwise.</returns>			
			bool RunIfTime(const uint32_t timestamp)
			{
				// Atomically read 'Enabled' and 'Delay' to prevent race conditions with ISRs.
				noInterrupts();
				const uint32_t delay = Enabled ? Delay : UINT32_MAX;
				interrupts();

				if ((delay == 0 || ((timestamp - LastRun) >= delay)))
				{
					Task->Run();
					LastRun = timestamp;

					return true;
				}
				else
				{
					return false;
				}
			}

			/// <summary>
			/// Calculates the time remaining until the next eligible run.
			/// Returns UINT32_MAX if the task is disabled.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>Milliseconds until next run, or UINT32_MAX if disabled.</returns>
			uint32_t TimeUntilNextRun(const uint32_t timestamp) const
			{
				if (!Enabled)
				{
					return UINT32_MAX;
				}
				else if (Delay == 0)
				{
					return 0;
				}
				else
				{
					const uint32_t elapsedSinceLastRun = timestamp - LastRun;

					if (elapsedSinceLastRun >= Delay)
					{
						return 0;
					}
					else
					{
						return Delay - elapsedSinceLastRun;
					}
				}
			}
		};
	}
}
#endif