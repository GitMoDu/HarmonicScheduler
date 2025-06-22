#ifndef _HARMONIC_ISCHEDULER_h
#define _HARMONIC_ISCHEDULER_h

#include "ITask.h"

namespace Harmonic
{
	/// <summary>
	/// Class interface for Harmonic Scheduler.
	/// </summary>
	class IScheduler
	{
		/// <summary>
		/// Task setup interface.
		/// </summary>
	public:
		/// <summary>
		/// Attach a task to the scheduler.
		/// Once added, a task cannot be removed.
		/// Tasks will have zero delay and be enabled by default.
		/// Adding tasks after runtime has started is not supported.
		/// </summary>
		/// <param name="task">Pointer to Harmonic ITask interface.</param>
		/// <param name="delay">Task execution period.</param>
		/// <param name="enabled">Task state.</param>
		/// <returns>True on success.</returns>
		virtual bool Attach(ITask* task, task_id_t& taskId, const uint32_t delay = 0, const bool enabled = true) { return false; }

		/// <summary>
		/// Task manage interface.
		/// </summary>
	public:
		/// <summary>
		/// Set task run delay period.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="delay">Task run delay period.</param>
		virtual void SetDelay(const uint8_t taskId, const uint32_t delay) {}

		/// <summary>
		/// Set Task Enable/Disable state.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="enabled">Task state.</param>
		virtual void SetEnabled(const uint8_t taskId, const bool enabled) {}

		/// <summary>
		/// Set task delay period and state.
		/// </summary>
		/// <param name="taskId">Valid TaskId returned by the scheduler.</param>
		/// <param name="delay">Task run delay period.</param>
		/// <param name="enabled">Task state.</param>
		virtual void Set(const uint8_t taskId, const uint32_t delay, const bool enabled) {}

	public:
		/// <summary>
		/// Forward scheduler time, to compensate for deep sleep.
		/// </summary>
		/// <param name="offset">Forward offset in milliseconds.</param>
		virtual void AdvanceTimestamp(const uint32_t offset) {}

		/// <summary>
		/// How long until the next scheduled run.
		/// </summary>
		/// <returns>Time in milliseconds.</returns>
		virtual uint32_t GetTimeUntilNextRun() const { return 0; }

	public:
		/// <summary>
		/// </summary>
		/// <returns>How many tasks are attached.</returns>
		virtual uint8_t GetTaskCount() const { return 0; }

		/// <summary>
		/// </summary>
		/// <returns>Max number of tasks that can be attached.</returns>
		virtual uint8_t GetMaxTaskCount() const { return 0; }
	};
}

#endif