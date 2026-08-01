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
		/// Supports dynamic binding and removal.
		/// </summary>
		struct TaskTracker
		{
			/// <summary>
			/// Pointer to the associated task to be managed.
			/// </summary>
			ITask* Task = nullptr;

			/// <summary>
			/// Minimum delay (in milliseconds) until next run call. 
			/// </summary>
			volatile uint32_t Delay = 0;

			/// <summary>
			/// Timestamp (in ms) of the last time the task was run.
			/// </summary>
			uint32_t LastRun = 0;

			/// <summary>
			/// Attachment-stable handle assigned by the owning task registry.
			/// </summary>
			task_handle_t Handle = TASK_INVALID_HANDLE;

			/// <summary>
			/// Indicates whether the task is enabled and eligible to run.
			/// </summary>
			volatile bool Enabled = false;

			/// <summary>
			/// Binds a task with a specified execution delay and enabled state, and initializes its last run timestamp.
			/// </summary>
			/// <param name="task">Pointer to the task to be bound.</param>
			/// <param name="delay">The execution delay for the task, in milliseconds.</param>
			/// <param name="handle">Attachment-stable handle assigned to the task.</param>
			/// <param name="enabled">Indicates whether the task should be enabled.</param>
			void BindTask(ITask* task, const uint32_t delay, const task_handle_t handle, const bool enabled)
			{
				// Atomically set the delay, enabled state and initialize LastRun.
				Platform::AtomicGuard guard;
				Handle = handle;
				Task = task;
				Delay = delay;
				Enabled = enabled;
				LastRun = Platform::GetTimestamp();
			}

			/// <summary>
			/// Unbinds the tracked task and resets its attachment state.
			/// </summary>
			void Unbind()
			{
				Platform::AtomicGuard guard;
				Handle = TASK_INVALID_HANDLE;
				Task = nullptr;
				Enabled = false;
				Delay = 0;
				LastRun = 0;
			}

			/// <summary>
			/// Determines whether the task is currently bound (has a valid handle).
			/// </summary>
			/// <returns>True if the task is bound, false otherwise.</returns>
			bool IsBound() const
			{
				return Handle != TASK_INVALID_HANDLE;
			}

			/// <summary>
			/// Runs the task if it is enabled and the delay period has elapsed since the last run.
			/// Updates LastRun if the task is executed.
			/// 
			/// Reads of 'Enabled' and 'Delay' are performed atomically by disabling interrupts
			/// during the read. This prevents race conditions with ISRs that may modify these
			/// variables, ensuring a consistent snapshot of their values.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>True if the task was run, false otherwise.</returns>			
			bool RunIfTime()
			{
				const uint32_t timestamp = Platform::GetTimestamp();

				// Check if the task should run based on its enabled state and delay.
				if (ShouldRun(timestamp))
				{
					// Update LastRun before running the callback.
					LastRun = timestamp;

					// Run the task callback.
					Task->Run();

					return true;
				}
				else
				{
					return false;
				}
			}

			/// <summary>
			/// Determines whether the task should run at the specified timestamp.
			/// Tipically used in conjunction with RunDirect() to allow the caller to control when the task is executed.
			/// </summary>
			/// <param name="timestamp">Current timestamp in milliseconds.</param>
			/// <returns>True if the task should run, false otherwise.</returns>
			bool ShouldRun(const uint32_t timestamp) const
			{
				// Use atomic protection for reading Enabled and Delay concurrently with potential ISR modifications.
				Platform::AtomicGuard guard;

				// Uses unsigned arithmetic for overflow safety.
				// The > comparison enforces late bias:
				// the task will only run after the scheduled delay has fully elapsed, never early.
				return Enabled && (Delay == 0 || ((timestamp - LastRun) > Delay));
			}

			/// <summary>
			/// Runs the task directly and updates its timing state.
			/// This is a privileged scheduler operation; the caller must have already
			/// confirmed that ShouldRun() is true for the supplied timestamp.
			/// LastRun is updated to the provided timestamp.
			/// </summary>
			/// <param name="timestamp">Timestamp captured by the scheduler's timing check.</param>
			void RunDirect(const uint32_t timestamp)
			{
				// Update LastRun before running the callback.
				LastRun = timestamp;

				// Run the task callback.
				Task->Run();
			}

			/// <summary>
			/// Sets the enabled/disabled state.
			/// Can be called at any time to enable or disable the task.
			/// </summary>
			/// <param name="enabled">New enabled state.</param>
			void SetEnabled(const bool enabled)
			{
				// On all supported platforms, reading/writing a bool is atomic.
				Enabled = enabled;
			}

			/// <summary>
			/// Sets the delay for the next run callback.
			/// Next run will occur after the specified delay has elapsed since the last run.
			/// If delay is being set from within a task's Run() method, 
			/// the next run will occur after the specified delay has elapsed since the Run() call start.
			/// </summary>
			/// <param name="delay">New delay in milliseconds.</param>
			void SetDelay(const uint32_t delay)
			{
				Platform::AtomicGuard guard;
				Delay = delay;
			}

			/// <summary>
			/// Sets the delay for the next run callback, relative to the current time.
			/// Next run will occur after the specified delay has elapsed since the current time.
			/// </summary>
			/// <param name="delay">New delay in milliseconds.</param>
			void SetDelayFromNow(const uint32_t delay)
			{
				LastRun = Platform::GetTimestamp();
				Platform::AtomicGuard guard;
				Delay = delay;
			}

			/// <summary>
			/// Immediately schedules the task to run on the next scheduler tick by resetting its delay and enabling it.
			/// </summary>
			void Wake()
			{
#if !defined(ARDUINO_ARCH_AVR) // On AVR without nested interrupts, atomic access is not needed.
				Platform::AtomicGuard guard;
#endif
				// Set the delay to 0 and enabled to true.
				Delay = 0;
				Enabled = true;
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
			/// Returns the current execution delay (in milliseconds) for the task.
			/// </summary>
			/// <returns>The delay in milliseconds.</returns>
			uint32_t GetDelay() const
			{
#if defined(HARMONIC_PLATFORM_ATOMIC_NARROW)
				// Use atomic protection.
				uint32_t delay;
				{
					Platform::AtomicGuard guard;
					delay = Delay;
				}

				return delay;
#else
				// 32-bit+ platforms: 32-bit access is atomic
				return Delay;
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
				// Atomically read the enabled state and delay.
				uint32_t delay;
				{
					Platform::AtomicGuard guard;
					if (!Enabled)
					{
						// Task is disabled, return UINT32_MAX to indicate no scheduled run.
						return UINT32_MAX;
					}
					delay = Delay;
				}

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
		};
	}
}
#endif