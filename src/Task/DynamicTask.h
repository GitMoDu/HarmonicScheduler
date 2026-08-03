#ifndef _HARMONIC_DYNAMIC_TASK_h
#define _HARMONIC_DYNAMIC_TASK_h

#include "AbstractTask.h"

namespace Harmonic
{
	/// <summary>
	/// Base class for tasks that can be dynamically registered and managed by a TaskRegistry, with flexible scheduling.
	/// - Designed for tasks that require flexible or frequent schedule changes.
	/// - Intended to be subclassed; override Run() to implement task logic.
	/// 
	/// Callability:
	///		- Attach, Detach: May be called at any time, but NOT from an ISR.
	///		- GetTaskHandle is safe to call at any time, will return TASK_INVALID_HANDLE if not registered.
	///		- All other methods are safe to call at any time after registration, including from an ISR.
	/// </summary>
	class DynamicTask : public AbstractTask
	{
	public:
		DynamicTask(TaskRegistry& registry) : AbstractTask(registry) {}

	protected:
		/// <summary>
		/// Sets the next run callback delay for this task in milliseconds.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelay(const uint32_t delay)
		{
			Registry.SetDelay(Handle, delay);
		}

		/// <summary>
		/// Sets the next run callback delay for this task relative to the current time in milliseconds.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelayFromNow(const uint32_t delay)
		{
			Registry.SetDelayFromNow(Handle, delay);
		}

		/// <summary>
		/// Returns the current delay (in milliseconds) for this task.
		/// Safe to call at any time after registration, including from an ISR.
		/// </summary>
		/// <returns>The delay in milliseconds.</returns>
		uint32_t GetDelay() const
		{
			return Registry.GetDelay(Handle);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// </summary>
		void WakeNow()
		{
			Registry.Wake(Handle);
		}
	};
}
#endif