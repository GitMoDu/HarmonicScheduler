#ifndef _HARMONIC_ABSTRACT_TASK_h
#define _HARMONIC_ABSTRACT_TASK_h

#include "../Model/ITask.h"
#include "../Model/TaskRegistry.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract class for task behaviours implemented by inheriting classes.
	/// Implements common functionality for attaching, detaching, enabling/disabling tasks.
	/// All task management is protected and routed through the stored handle, which is stable for the duration of an attachment.
	/// Exposes only a common TaskHandle getter for profiling and debugging.
	///
	/// - Maintains a reference to a TaskRegistry and its own attachment-stable handle.
	/// - Hides handle parameters from normal task code; methods route operations through the stored handle.
	/// - Allows the task to attach/detach, enable/disable itself setup/runtime.
	/// - Abstract class, no delay management.
	///
	/// Callability:
	///		- Attach, Detach: May be called at any time, but NOT from an ISR.
	///		- GetTaskHandle is safe to call at any time, will return TASK_INVALID_HANDLE if not registered.
	///		- All other methods are safe to call at any time after registration, including from an ISR.
	/// </summary>
	class AbstractTask : public ITask
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
		AbstractTask(TaskRegistry& registry)
			: ITask()
			, Registry(registry)
		{}

		/// <summary>
		/// Returns the handle assigned to the current registry attachment.
		/// Safe to call at any time.
		/// </summary>
		/// <returns>Current attachment handle, or TASK_INVALID_HANDLE if not registered.</returns>
		task_handle_t GetTaskHandle() const
		{
			return Handle;
		}

	protected:
		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// May be called at any time, but NOT from an ISR.
		/// </summary>
		/// <param name="delay">Initial execution delay in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if attachment succeeded or is already attached, false otherwise.</returns>
		bool Attach(const uint32_t delay = 0, const bool enabled = true)
		{
			Handle = Registry.Attach(this, delay, enabled);

			return Handle != TASK_INVALID_HANDLE;
		}

		/// <summary>
		/// Removes this task from the registry.
		/// May be called at any time, but NOT from an ISR.
		/// After removal, the task will no longer be scheduled or run.
		/// </summary>
		void Detach()
		{
			if (Handle == TASK_INVALID_HANDLE)
				return;

			Registry.Detach(Handle);
			Handle = TASK_INVALID_HANDLE;
		}

		bool IsAttached() const
		{
			return Handle != TASK_INVALID_HANDLE && Registry.TaskExists(this);
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
		/// Returns true if this task is currently enabled in the registry.
		/// Safe to call at any time after registration.
		/// </summary>
		bool IsEnabled() const
		{
			return Registry.IsEnabled(Handle);
		}
	};
}
#endif