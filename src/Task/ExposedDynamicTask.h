#ifndef _HARMONIC_EXPOSED_DYNAMIC_TASK_h
#define _HARMONIC_EXPOSED_DYNAMIC_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// Wrapper for a dynamic task that exposes all task management.
	/// - Inherits all scheduling and registry features from DynamicTask.
	/// - Intended for composition; inheriting classes must provide the run callback.
	///
	/// Callability:
	///		- Attach, Detach: May be called at any time, but NOT from an ISR.
	///		- GetTaskHandle is safe to call at any time, will return TASK_INVALID_HANDLE if not registered.
	///		- All other methods are safe to call at any time after registration, including from an ISR.
	/// </summary>
	class ExposedDynamicTask : public DynamicTask
	{
	public:
		/// <summary>
		/// </summary>
		/// <param name="registry">Reference to the TaskRegistry for scheduling and management.</param>
		ExposedDynamicTask(TaskRegistry& registry)
			: DynamicTask(registry)
		{}

		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// May be called at any time, but NOT from an ISR.
		/// </summary>
		/// <param name="delay">Initial execution delay in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if attachment succeeded or is already attached, false otherwise.</returns>
		bool Attach(const uint32_t delay = 0, const bool enabled = true)
		{
			return DynamicTask::Attach(delay, enabled);
		}

		/// <summary>
		/// Removes this task from the registry.
		/// May be called at any time after registration, but NOT from an ISR.
		/// After removal, the task will no longer be scheduled or run.
		/// </summary>
		void Detach()
		{
			DynamicTask::Detach();
		}

		/// <summary>
		/// Returns the handle assigned to the current registry attachment.
		/// Safe to call at any time after registration.
		/// </summary>
		/// <returns>Current attachment handle, or TASK_INVALID_HANDLE if not registered.</returns>
		task_handle_t GetTaskHandle() const
		{
			return DynamicTask::GetTaskHandle();
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			DynamicTask::SetEnabled(enabled);
		}

		/// <summary>
		/// Returns true if this task is currently enabled in the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		bool IsEnabled() const
		{
			return DynamicTask::IsEnabled();
		}

		/// <summary>
		/// Sets the next run callback delay for this task in milliseconds.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelay(const uint32_t delay)
		{
			DynamicTask::SetDelay(delay);
		}

		/// <summary>
		/// Sets the next run callback delay for this task relative to the current time in milliseconds.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelayFromNow(const uint32_t delay)
		{
			DynamicTask::SetDelayFromNow(delay);
		}

		/// <summary>
		/// Returns the current delay (in milliseconds) for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <returns>The delay in milliseconds.</returns>
		uint32_t GetDelay() const
		{
			return DynamicTask::GetDelay();
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		void WakeNow()
		{
			DynamicTask::WakeNow();
		}
	};
}
#endif