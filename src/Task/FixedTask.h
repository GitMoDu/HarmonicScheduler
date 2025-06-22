#ifndef _HARMONIC_FIXED_TASK_h
#define _HARMONIC_FIXED_TASK_h

#include "../Model/ITask.h"
#include "../Model/IScheduler.h"

namespace Harmonic
{
	/// <summary>
	/// Harmonic Cooperative abstract fixed task, abstracts only TaskId usage.
	/// Classes can override Run().
	/// Execution can only be managed externally, as it only stores a reference to the TaskId.
	/// </summary>
	class FixedTask : public ITask
	{
	private:
		task_id_t TaskId = UINT8_MAX;

	public:
		FixedTask() : ITask()
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
		/// Attach task to scheduler and setup state and period.
		/// </summary>
		/// <param name="delay">Task execution period.</param>
		/// <param name="enabled">Task state on attach.</param>
		/// <returns>True on success.</returns>
		bool AttachTask(IScheduler& scheduler, const uint32_t delay = 0, const bool enabled = true)
		{
			return scheduler.Attach(this, TaskId, delay, enabled);
		}
	};
}
#endif