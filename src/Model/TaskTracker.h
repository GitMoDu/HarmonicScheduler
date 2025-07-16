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
			/// Minimum period (in milliseconds) between consecutive task runs.
			/// </summary>
			volatile uint32_t Period = 0;

			/// <summary>
			/// Timestamp (in ms) of the last time the task was run.
			/// </summary>
			uint32_t LastRun = 0;

			/// <summary>
			/// Indicates whether the task is enabled and eligible to run.
			/// </summary>
			volatile bool Enabled = false;

			/// <summary>
			/// Binds a task with a specified execution period and enabled state, and initializes its last run timestamp.
			/// </summary>
			/// <param name="task">Pointer to the task to be bound.</param>
			/// <param name="period">The execution period for the task, in milliseconds.</param>
			/// <param name="enabled">Indicates whether the task should be enabled.</param>
			void BindTask(ITask* task, const uint32_t period, const bool enabled)
			{
				// Atomically set the task, period, enabled state and initialize LastRun.
				Platform::AtomicGuard guard;
				Task = task;
				Period = period;
				Enabled = enabled;
				if (enabled)
				{
					LastRun = Platform::GetTimestamp();
				}
			}

			/// <summary>
			/// Runs the task if it is enabled and the delay period has elapsed since the last run.
			/// Updates LastRun if the task is executed.
			/// 
			/// Reads of 'Enabled' and 'Period' are performed atomically by disabling interrupts
			/// during the read. This prevents race conditions with ISRs that may modify these
			/// variables, ensuring a consistent snapshot of their values.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>True if the task was run, false otherwise.</returns>			
			bool RunIfTime()
			{
				// Atomically read 'Enabled' and 'Period' to prevent race conditions with ISRs.
				uint32_t period = UINT32_MAX;
				{
					Platform::AtomicGuard guard;
					if (Enabled)
						period = Period;
				}

				const uint32_t timestamp = Platform::GetTimestamp();
				const uint32_t elapsed = timestamp - LastRun;

				// Run the task if the period has elapsed.
				// Uses unsigned arithmetic for overflow safety.
				// The > comparison enforces late bias:
				// the task will only run after the scheduled period has fully elapsed, never early.
				if (period == 0 || (elapsed > period))
				{
					Task->Run();

					// If the scheduler was delayed and we missed more than one period,
					// resynchronize LastRun to the current timestamp to avoid multiple rapid catch-up runs.
					if (period > 0 && (elapsed >> 1) > period)
					{
						// If we missed more than one period (scheduler delayed), resync LastRun to now.
						LastRun = timestamp;
					}
					else
					{
						LastRun += period;
					}

					return true;
				}
				else
				{
					return false;
				}
			}

			/// <summary>
			/// Sets the run period period.
			/// </summary>
			/// <param name="period">New period period in milliseconds.</param>
			void SetPeriod(const uint32_t period)
			{
#if !defined(UINTPTR_MAX)  || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				// Use atomic protection on platforms with pointer size < 32 bits,
				// or if UINTPTR_MAX is not defined (safe fallback).
				Platform::AtomicGuard guard;
				Period = period;
#else
				// 32-bit+ platforms: 32-bit access is atomic
				Period = period;
#endif
			}

			/// <summary>
			/// Sets the enabled/disabled state.
			/// </summary>
			/// <param name="enabled">New enabled state.</param>
			void SetEnabled(const bool enabled)
			{
				// Atomically update the enabled state, updating LastRun if enabling the task.
				Platform::AtomicGuard guard;
				if (enabled && !Enabled)
				{
					LastRun = Platform::GetTimestamp();
				}
				Enabled = enabled;
			}

			/// <summary>
			/// Sets both the run period and enabled state.
			/// For the purposes of immediatelly waking up the task, use WakeFromISR() instead.
			/// </summary>
			/// <param name="period">New period period in milliseconds.</param>
			/// <param name="enabled">New enabled state.</param>
			void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
			{
				// Atomically update the period and enabled state, updating LastRun if enabling the task.
				Platform::AtomicGuard guard;
				if (enabled && !Enabled)
				{
					LastRun = Platform::GetTimestamp();
				}
				Period = period;
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
			/// Returns the current period (in milliseconds) for the task.
			/// </summary>
			/// <returns>The period in milliseconds.</returns>
			uint32_t GetPeriod() const
			{
#if !defined(UINTPTR_MAX)  || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				uint32_t period;
				// Use atomic protection on platforms with pointer size < 32 bits,
				// or if UINTPTR_MAX is not defined (safe fallback).
				{
					Platform::AtomicGuard guard;
					period = Period;
				}

				return period;
#else
				// 32-bit+ platforms: 32-bit access is atomic
				return Period;
#endif
			}

			/// <summary>
			/// Calculates the time remaining until the next eligible run.
			/// Returns UINT32_MAX if the task is disabled.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>Milliseconds until next run, or UINT32_MAX if disabled.</returns>
			uint32_t TimeUntilNextRun(const uint32_t timestamp) const
			{
				// Atomically read 'Enabled' and 'Period' to prevent race conditions with ISRs.
				uint32_t period;
				{
					Platform::AtomicGuard guard;
					period = Enabled ? Period : UINT32_MAX;
				}

				if (period == 0)
				{
					return 0;
				}
				else if (period == UINT32_MAX)
				{
					return UINT32_MAX;
				}
				else
				{
					const uint32_t elapsedSinceLastRun = timestamp - LastRun;

					if (elapsedSinceLastRun >= period)
					{
						return 0;
					}
					else
					{
						return period - elapsedSinceLastRun;
					}
				}
			}
		};
	}
}
#endif