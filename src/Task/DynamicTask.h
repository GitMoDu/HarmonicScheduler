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
	/// - Allows the task to adjust its own scheduling (delay, enable/disable) at runtime.
	/// - Designed for tasks that require flexible or frequent schedule changes.
	/// - Intended to be subclassed; override Run() to implement task logic.
	/// </summary>
	class DynamicTask : public ITask
	{
	private:
		/// <summary>
		/// Reference to the registry for managing this task.
		/// </summary>
		TaskRegistry& Registry;

	private:
		/// <summary>
		/// Unique identifier for this task within the registry.
		/// Set during registration; UINT8_MAX if unregistered.
		/// </summary>
		task_id_t TaskId = UINT8_MAX;

	public:
		/// <summary>
		/// Constructs a DynamicTask with a reference to the registry.
		/// </summary>
		/// <param name="registry">TaskRegistry for the task.</param>
		DynamicTask(TaskRegistry& registry) : ITask()
			, Registry(registry)
		{
		}

		/// <summary>
		/// Returns the unique task ID assigned by the registry.
		/// </summary>
		/// <returns>Task ID, or UINT8_MAX if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return TaskId;
		}

	public:
		/// <summary>
		/// Sets the execution period (delay) for this task.
		/// </summary>
		/// <param name="delay">New execution period in milliseconds.</param>
		void SetTaskDelay(const uint32_t delay)
		{
			Registry.SetDelay(TaskId, delay);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetTaskEnabled(const bool enabled)
		{
			Registry.SetEnabled(TaskId, enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// </summary>
		/// <param name="delay">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetTaskDelayEnabled(const uint32_t delay, const bool enabled)
		{
			Registry.SetDelayEnabled(TaskId, delay, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediatelly.
		/// This method is safe to call from an ISR.
		/// </summary>
		void WakeTaskFromISR()
		{
			Registry.WakeFromISR(TaskId);
		}

	protected:
		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// </summary>
		/// <param name="delay">Initial execution period in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if registration succeeded, false otherwise.</returns>
		bool AttachTask(const uint32_t delay = 0, const bool enabled = true)
		{
			return Registry.AttachTask(this, TaskId, delay, enabled);
		}
	};
}
#endif