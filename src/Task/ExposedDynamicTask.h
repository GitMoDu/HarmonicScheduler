#ifndef _HARMONIC_EXPOSED_DYNAMIC_TASK_h
#define _HARMONIC_EXPOSED_DYNAMIC_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract wrapper for a dynamic task that exposes all task management.
	///
	/// - Inherits all scheduling and registry features from DynamicTask.
	/// - Intended for composition; inheriting classes must provide the run callback.
	///
	/// Callability:
	///   - Attach, Detach: May be called at any time after construction, but NOT from an ISR.
	///   - SetPeriod, SetEnabled, SetPeriodAndEnabled, WakeFromISR: Safe to call at any time after registration, including from an ISR.
	///   - GetTaskId, IsEnabled, GetPeriod: Safe to call at any time after registration.
	/// </summary>
	class ExposedDynamicTask : public DynamicTask
	{
	public:
		/// <summary>
		/// </summary>
		/// <param name="registry">Reference to the TaskRegistry for scheduling and management.</param>
		ExposedDynamicTask(TaskRegistry& registry)
			: DynamicTask(registry)
		{
		}

		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// May be called at any time after construction, but NOT from an ISR.
		/// </summary>
		/// <param name="period">Initial execution period in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if registration succeeded, false otherwise.</returns>
		bool Attach(const uint32_t period = 0, const bool enabled = true)
		{
			return DynamicTask::Attach(period, enabled);
		}

		/// <summary>
		/// Returns the unique task ID assigned by the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		/// <returns>Task ID, or TASK_INVALID_ID if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return DynamicTask::GetTaskId();
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
		/// Returns the current period for this task in milliseconds.
		/// Safe to call at any time after registration.
		/// </summary>
		uint32_t GetPeriod() const
		{
			return DynamicTask::GetPeriod();
		}

		/// <summary>
		/// Sets the execution period for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		void SetPeriod(const uint32_t period)
		{
			DynamicTask::SetPeriod(period);
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
		/// Sets both the execution period and enabled state for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
		{
			DynamicTask::SetPeriodAndEnabled(period, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		void WakeFromISR()
		{
			DynamicTask::WakeFromISR();
		}
	};
}
#endif