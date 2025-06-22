#ifndef _HARMONIC_DYNAMIC_TASK_h
#define _HARMONIC_DYNAMIC_TASK_h

#include "../Model/ITask.h"
#include "../Model/IScheduler.h"

namespace Harmonic
{
	/// <summary>
	/// Harmonic Cooperative abstract task, abstracts Scheduler management and TaskId usage.
	/// Classes can override Run().
	/// SetDelay() and SetEnabled() can be used to manage the execution.
	/// Stores a reference to the scheduler and the TaskId.
	/// </summary>
	class DynamicTask : public ITask
	{
	protected:
		IScheduler& Harmony;

	private:
		task_id_t TaskId = UINT8_MAX;

	public:
		DynamicTask(IScheduler& scheduler) : ITask()
			, Harmony(scheduler)
		{
		}

		task_id_t GetTaskId() const
		{
			return TaskId;
		}

	public:
		/// <summary>
		/// Execute callback.
		/// Total execution time must be under 1 ms.
		/// </summary>
		virtual void Run() {}

	public:
		/// <summary>
		/// Set task execution period.
		/// </summary>
		/// <param name="delay">Task execution period</param>
		void SetDelay(const uint32_t delay)
		{
			Harmony.SetDelay(TaskId, delay);
		}

		/// <summary>
		/// Enable/Disable task from execution.
		/// </summary>
		/// <param name="enabled">Task state.</param>
		void SetEnabled(const bool enabled)
		{
			Harmony.SetEnabled(TaskId, enabled);
		}

		/// <summary>
		/// Set task execution period and Enable/Disable task from execution.
		/// </summary>
		/// <param name="delay">Task execution period.</param>
		/// <param name="enabled">Task state.</param>
		void Set(const uint32_t delay, const bool enabled)
		{
			Harmony.Set(TaskId, delay, enabled);
		}

	protected:
		/// <summary>
		/// Attach task to scheduler and setup state and period.
		/// </summary>
		/// <param name="delay">Task execution period.</param>
		/// <param name="enabled">Task state on attach.</param>
		/// <returns>True on success.</returns>
		bool AttachTask(const uint32_t delay = 0, const bool enabled = true)
		{
			return Harmony.Attach(this, TaskId, delay, enabled);
		}
	};
}
#endif