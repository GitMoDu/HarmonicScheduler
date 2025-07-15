#ifndef _HARMONIC_EXPOSED_DYNAMIC_TASK_h
#define _HARMONIC_EXPOSED_DYNAMIC_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract wrapper for a dynamic task that exposes all task management.
	///
	/// - Inherits all scheduling and registry features from DynamicTask.
	/// - Requires composition by not implementing Run() method.
	/// </summary>
	class ExposedDynamicTask : public DynamicTask
	{
	public:
		/// <summary>
		/// Constructs a DynamicTaskWrapper.
		/// </summary>
		/// <param name="registry">Reference to the TaskRegistry for scheduling and management.</param>
		ExposedDynamicTask(TaskRegistry& registry)
			: DynamicTask(registry)
		{
		}

		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// Should only be called during setup/initialization, before the scheduler starts.
		/// Do not call after the scheduler has started. Do not call from an ISR.
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
		/// </summary>
		/// <returns>Task ID, or UINT8_MAX if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return DynamicTask::GetTaskId();
		}

		/// <summary>
		/// Returns true if this task is currently enabled in the registry.
		/// </summary>
		bool IsEnabled() const
		{
			return DynamicTask::IsEnabled();
		}

		/// <summary>
		/// Returns the current period for this task in milliseconds.
		/// </summary>
		uint32_t GetPeriod() const
		{
			return DynamicTask::GetPeriod();
		}

		/// <summary>
		/// Sets the execution period for this task.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		void SetPeriod(const uint32_t period)
		{
			DynamicTask::SetPeriod(period);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			DynamicTask::SetEnabled(enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
		{
			DynamicTask::SetPeriodAndEnabled(period, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call from an ISR.
		/// </summary>
		void WakeFromISR()
		{
			DynamicTask::WakeFromISR();
		}
	};
}
#endif