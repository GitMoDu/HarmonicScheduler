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
	/// - Allows the task to adjust its own scheduling (period, enable/disable) at runtime.
	/// - Designed for tasks that require flexible or frequent schedule changes.
	/// - Intended to be subclassed; override Run() to implement task logic.
	/// 
	/// Thread/ISR Safety:
	///   - Only WakeFromISR() is safe to call from an ISR.
	///   - All other methods are safe to call after setup/registration, but must NOT be called from an ISR.
	///   - Attach() should only be called during initialization/setup, not from an ISR or after the scheduler starts.
	///   - Simple reads/writes of bools (e.g., Enabled) are atomic and ISR-safe, but registry methods may not be.
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
		/// Set during registration; UINT8_MAX if unregistered.
		/// </summary>
		task_id_t Id = UINT8_MAX;

	public:
		/// <summary>
		/// Constructs a DynamicTask with a reference to the registry.
		/// </summary>
		/// <param name="registry">TaskRegistry for the task.</param>
		DynamicTask(TaskRegistry& registry) : ITask()
			, Registry(registry)
		{
		}

	protected:
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
			return Registry.Attach(this, period, enabled);
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
		/// Safe to call at any time after registration.
		/// </summary>
		/// <returns>Task ID, or UINT8_MAX if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return Id;
		}

		/// <summary>
		/// Returns the current period for this task in milliseconds.
		/// Safe to call after registration, but NOT from an ISR.
		/// </summary>
		uint32_t GetPeriod() const
		{
			return Registry.GetPeriod(Id);
		}

		/// <summary>
		/// Sets the execution period for this task.
		/// Safe to call at any time after registration.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		void SetPeriod(const uint32_t period)
		{
			Registry.SetPeriod(Id, period);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			Registry.SetEnabled(Id, enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// Safe to call at any time after registration.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
		{
			Registry.SetPeriodAndEnabled(Id, period, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Use this call from an ISR, as it is designed for ISR context and is faster than other methods.
		/// Safe to call at any time after registration.
		/// </summary>
		void WakeFromISR()
		{
			Registry.WakeFromISR(Id);
		}
	};
}
#endif