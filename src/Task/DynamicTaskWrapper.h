#ifndef _HARMONIC_DYNAMIC_TASK_WRAPPER_h
#define _HARMONIC_DYNAMIC_TASK_WRAPPER_h

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// Wrapper for a dynamic Harmonic task that delegates execution to an external ITask.
	///
	/// - Inherits all scheduling and registry features from DynamicTask.
	/// - Allows composition by holding a pointer to an external ITask ("Runner").
	/// - When Run() is called, forwards execution to Runner->Run() if Runner is set.
	/// - Useful for scenarios where task logic is provided by another class or needs to be swapped at runtime.
	///
	/// Usage:
	///   - Construct with a TaskRegistry and optional ITask pointer.
	///   - Set or change the underlying task at any time using SetTask().
	///   - Register and schedule as a normal DynamicTask.
	/// </summary>
	class DynamicTaskWrapper final : public DynamicTask
	{
	private:
		/// <summary>
		/// Pointer to the external ITask to be executed.
		/// If nullptr, Run() does nothing.
		/// </summary>
		ITask* Runner;

	public:
		/// <summary>
		/// Constructs a DynamicTaskWrapper.
		/// </summary>
		/// <param name="registry">Reference to the TaskRegistry for scheduling and management.</param>
		/// <param name="task">Optional pointer to the ITask to delegate execution to.</param>
		DynamicTaskWrapper(TaskRegistry& registry, ITask* task = nullptr)
			: DynamicTask(registry)
			, Runner(task)
		{
		}

		/// <summary>
		/// Registers this task with the registry and sets its initial schedule.
		/// Should only be called during setup/initialization, before the scheduler starts.
		/// Do not call after the scheduler has started. Do not call from an ISR.
		/// </summary>
		/// <param name="delay">Initial execution period in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True if registration succeeded, false otherwise.</returns>
		bool Attach(const uint32_t delay = 0, const bool enabled = true)
		{
			return DynamicTask::Attach(delay, enabled);
		}

		/// <summary>
		/// Sets or replaces the underlying ITask to be executed.
		/// </summary>
		/// <param name="task">Pointer to the new ITask. Can be nullptr to disable execution.</param>
		void SetTask(ITask* task)
		{
			Runner = task;
		}

		/// <summary>
		/// Executes the wrapped ITask's Run() method if Runner is set.
		/// Overrides DynamicTask::Run().
		/// </summary>
		void Run() final
		{
			if (Runner != nullptr)
			{
				Runner->Run();
			}
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
		/// Returns the current delay (period) for this task in milliseconds.
		/// </summary>
		uint32_t GetDelay() const
		{
			return DynamicTask::GetDelay();
		}

		/// <summary>
		/// Sets the execution period (delay) for this task.
		/// </summary>
		/// <param name="delay">New execution period in milliseconds.</param>
		void SetDelay(const uint32_t delay)
		{
			DynamicTask::SetDelay(delay);
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
		/// <param name="delay">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetDelayEnabled(const uint32_t delay, const bool enabled)
		{
			DynamicTask::SetDelayEnabled(delay, enabled);
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