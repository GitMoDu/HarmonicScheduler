#ifndef _HARMONIC_TASK_TRACKER_h
#define _HARMONIC_TASK_TRACKER_h

#include "ITask.h"
#include "../Platform/Atomic.h"

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
				uint32_t delay;
				{
					Platform::AtomicGuard lock;
					delay = Enabled ? Delay : UINT32_MAX;
				}

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
			/// Sets the run delay period.
			/// </summary>
			/// <param name="delay">New delay period in milliseconds.</param>
			void SetDelay(const uint32_t delay)
			{
#if !defined(UINTPTR_MAX)  || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				// Use atomic protection on platforms with pointer size < 32 bits,
				// or if UINTPTR_MAX is not defined (safe fallback).
				Platform::Guard lock;
				Delay = delay;
#else
				// 32-bit+ platforms: 32-bit access is atomic
				Delay = delay;
#endif
			}

			/// <summary>
			/// Sets the enabled/disabled state.
			/// </summary>
			/// <param name="enabled">New enabled state.</param>
			void SetEnabled(const bool enabled)
			{
				// On all supported platforms, reading/writing a bool is atomic.
				Enabled = enabled;
			}

			/// <summary>
			/// Returns whether the task is currently enabled.
			/// </summary>
			/// <returns>True if the task is enabled, false otherwise.</returns>
			bool IsEnabled() const
			{
				// On all supported platforms, reading/writing a bool is atomic.
				return Enabled;
			}

			/// <summary>
			/// Returns the current delay period (in milliseconds) for the task.
			/// </summary>
			/// <returns>The delay period in milliseconds.</returns>
			uint32_t GetDelay() const
			{
#if !defined(UINTPTR_MAX)  || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				uint32_t delay;
				// Use atomic protection on platforms with pointer size < 32 bits,
				// or if UINTPTR_MAX is not defined (safe fallback).
				{
					Platform::Guard lock;
					delay = Delay;
				}

				return delay;
#else
				// 32-bit+ platforms: 32-bit access is atomic
				return Delay;
#endif
			}

			/// <summary>
			/// Sets both the run delay period and enabled state.
			/// </summary>
			/// <param name="delay">New delay period in milliseconds.</param>
			/// <param name="enabled">New enabled state.</param>
			void SetDelayEnabled(const uint32_t delay, const bool enabled)
			{
				// Atomically set both Delay and Enabled to prevent race conditions with ISRs.
				Platform::AtomicGuard lock;
				Delay = delay;
				Enabled = enabled;
			}

			/// <summary>
			/// Calculates the time remaining until the next eligible run.
			/// Returns UINT32_MAX if the task is disabled.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>Milliseconds until next run, or UINT32_MAX if disabled.</returns>
			uint32_t TimeUntilNextRun(const uint32_t timestamp) const
			{
				// Atomically read 'Enabled' and 'Delay' to prevent race conditions with ISRs.
				uint32_t delay;
				{
					Platform::AtomicGuard lock;
					delay = Enabled ? Delay : UINT32_MAX;
				}

				if (delay == 0)
				{
					return 0;
				}
				else if (delay == UINT32_MAX)
				{
					return UINT32_MAX;
				}
				else
				{
					const uint32_t elapsedSinceLastRun = timestamp - LastRun;

					if (elapsedSinceLastRun >= delay)
					{
						return 0;
					}
					else
					{
						return delay - elapsedSinceLastRun;
					}
				}
			}
		};
	}
}
#endif