#ifndef _HARMONIC_DYNAMIC_TASK_h
#define _HARMONIC_DYNAMIC_TASK_h

#include "../Model/ITask.h"
#include "../Model/TaskRegistry.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract base class for a cooperative, dynamically managed task.
	///
	/// - Maintains a reference to a TaskRegistry and its own unique task ID.
	/// - Allows the task to attach/detach itself and adjust its own scheduling (period, enable/disable) at setup/runtime.
	/// - Designed for tasks that require flexible or frequent schedule changes.
	/// - Intended to be subclassed; override Run() to implement task logic.
	///
	/// Thread/ISR Safety:
	///   - Attach, Detach: May be called at any time, but NOT from an ISR.
	///   - SetPeriod, SetEnabled, SetPeriodAndEnabled, WakeFromISR: Safe to call at any time after registration, including from an ISR.
	///   - GetTaskId, IsEnabled, GetPeriod: Safe to call at any time after registration.
	/// </summary>
	class DynamicTask : public ITask
	{
	protected:
		/// <summary>
		/// Reference to the registry for managing this task.
		/// </summary>
		TaskRegistry& Registry;

		/// <summary>
		/// Unique identifier for this task within the registry.
		/// Set during registration; TASK_INVALID_ID if unregistered.
		/// </summary>
		task_id_t Id = TASK_INVALID_ID;

	public:
		/// <summary>
		/// Constructs a DynamicTask with a reference to the registry.
		/// </summary>
		/// <param name="registry">TaskRegistry for the task.</param>
		DynamicTask(TaskRegistry& registry) : ITask(), Registry(registry) {}

		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// May be called at any time, but NOT from an ISR.
		/// </summary>
		/// <param name="period">Initial execution period in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if registration succeeded, false otherwise.</returns>
		bool Attach(const uint32_t period = 0, const bool enabled = true)
		{
			return Registry.Attach(this, period, enabled);
		}

		/// <summary>
		/// Removes this task from the registry.
		/// May be called at any time, but NOT from an ISR.
		/// After removal, the task will no longer be scheduled or run.
		/// </summary>
		/// <returns>True if removal succeeded, false otherwise.</returns>
		bool Detach()
		{
			if (Id == TASK_INVALID_ID)
				return false;
			const bool result = Registry.Detach(Id);
			return result && Id == TASK_INVALID_ID;
		}

		void OnTaskIdUpdated(const task_id_t taskId) final
		{
			// Store the assigned task ID for later use.
			Id = taskId;
		}

		/// <summary>
		/// Returns true if this task is currently enabled in the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		bool IsEnabled() const
		{
			return Registry.IsEnabled(Id);
		}

		/// <summary>
		/// Returns the unique task ID assigned by the registry.
		/// Safe to call at any time.
		/// </summary>
		/// <returns>Task ID, or TASK_INVALID_ID if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return Id;
		}

		/// <summary>
		/// Returns the current period for this task in milliseconds.
		/// Safe to call at any time after registration.
		/// </summary>
		uint32_t GetPeriod() const
		{
			return Registry.GetPeriod(Id);
		}

		/// <summary>
		/// Sets the execution period for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		void SetPeriod(const uint32_t period)
		{
			Registry.SetPeriod(Id, period);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			Registry.SetEnabled(Id, enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
		{
			Registry.SetPeriodAndEnabled(Id, period, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		void WakeFromISR()
		{
			Registry.WakeFromISR(Id);
		}
	};
}
#endif