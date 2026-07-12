#ifndef _HARMONIC_DYNAMIC_TASK_h
#define _HARMONIC_DYNAMIC_TASK_h

#include "../Model/ITask.h"
#include "../Model/TaskRegistry.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract base class for a cooperative, dynamically managed task.
	///
	/// - Maintains a reference to a TaskRegistry and its own attachment-stable handle.
	/// - Hides handle parameters from normal task code; methods route operations through the stored handle.
	/// - Allows the task to attach/detach itself and adjust its own scheduling (period, enable/disable) at setup/runtime.
	/// - Designed for tasks that require flexible or frequent schedule changes.
	/// - Intended to be subclassed; override Run() to implement task logic.
	///
	/// Thread/ISR Safety:
	///   - Attach, Detach: May be called at any time, but NOT from an ISR.
	///   - SetPeriod, SetEnabled, SetPeriodAndEnabled, WakeFromISR: Safe to call at any time after registration, including from an ISR.
	///   - GetHandle, IsEnabled, GetPeriod: Safe to call at any time after registration.
	/// </summary>
	class DynamicTask : public ITask
	{
	protected:
		/// <summary>
		/// Reference to the registry for managing this task.
		/// </summary>
		TaskRegistry& Registry;

		/// <summary>
		/// Handle for the current registry attachment.
		/// Stable while attached and invalidated by a successful Detach().
		/// Handle values may be recycled by the registry after removal; this is
		/// not a lifetime-unique task identifier.
		/// </summary>
		task_handle_t Handle = TASK_INVALID_HANDLE;

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
		/// <returns>Handle stable for this attachment, or TASK_INVALID_HANDLE on failure.</returns>
		task_handle_t Attach(const uint32_t period = 0, const bool enabled = true)
		{
			const task_handle_t attachedHandle = Registry.Attach(this, period, enabled);
			if (attachedHandle != TASK_INVALID_HANDLE)
			{
				Handle = attachedHandle;
			}

			return attachedHandle;
		}

		/// <summary>
		/// Removes this task from the registry.
		/// May be called at any time, but NOT from an ISR.
		/// After removal, the task will no longer be scheduled or run.
		/// </summary>
		/// <returns>True if removal succeeded, false otherwise.</returns>
		bool Detach()
		{
			if (Handle == TASK_INVALID_HANDLE)
				return false;

			const bool result = Registry.Detach(Handle);
			if (result)
			{
				Handle = TASK_INVALID_HANDLE;
			}

			return result;
		}

		/// <summary>
		/// Returns true if this task is currently enabled in the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		bool IsEnabled() const
		{
			return Registry.IsEnabled(Handle);
		}

		/// <summary>
		/// Returns the handle assigned to the current registry attachment.
		/// Safe to call at any time.
		/// </summary>
		/// <returns>Current attachment handle, or TASK_INVALID_HANDLE if not registered.</returns>
		task_handle_t GetHandle() const
		{
			return Handle;
		}

		/// <summary>
		/// Returns the current period for this task in milliseconds.
		/// Safe to call at any time after registration.
		/// </summary>
		uint32_t GetPeriod() const
		{
			return Registry.GetPeriod(Handle);
		}

		/// <summary>
		/// Sets the execution period for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		void SetPeriod(const uint32_t period)
		{
			Registry.SetPeriod(Handle, period);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			Registry.SetEnabled(Handle, enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="period">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetPeriodAndEnabled(const uint32_t period, const bool enabled)
		{
			Registry.SetPeriodAndEnabled(Handle, period, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		void WakeFromISR()
		{
			Registry.WakeFromISR(Handle);
		}
	};
}
#endif