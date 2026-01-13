#ifndef _HARMONIC_ITASK_h
#define _HARMONIC_ITASK_h

#include "../Platform/Platform.h"

namespace Harmonic
{
	/// <summary>
	/// Abstract interface for a cooperative task in the Harmonic framework.
	/// 
	/// Classes implementing ITask must provide a Run() method containing the task's run callback.
	/// Tasks are expected to be non-blocking and complete execution quickly (typically under 1 ms).
	/// </summary>
	struct ITask
	{
		/// <summary>
		/// Virtual destructor for the ITask interface.
		/// </summary>
		virtual ~ITask() = default;

		/// <summary>
		/// Task execution callback.
		/// Override this method to implement the task's behavior.
		/// The method should return quickly and must not block.
		/// </summary>
		virtual void Run() = 0;

		/// <summary>
		/// Called when a task's ID is updated. Implementations must store their new task ID, if any scheduling changes are needed.
		/// </summary>
		/// <param name="taskId">The new task ID value.</param>
		virtual void OnTaskIdUpdated(const task_id_t taskId) = 0;
	};
}
#endif