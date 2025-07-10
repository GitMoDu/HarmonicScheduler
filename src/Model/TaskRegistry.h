#ifndef _HARMONIC_TASKS_MANAGER_h
#define _HARMONIC_TASKS_MANAGER_h

#include "ITask.h"
#include "TaskTracker.h"
#include "../Platform/Platform.h"

namespace Harmonic
{
	/// <summary>
	/// TaskRegistry provides dynamic registration and management of cooperative tasks.
	/// 
	/// Stores pointers to ITask implementations in a dynamically allocated array of TaskTracker objects.
	/// Supports adding, clearing, and querying tasks, as well as updating their delay and enabled state.
	/// </summary>
	class TaskRegistry
	{
	private:
		/// <summary>
		/// Externally allocated array of TaskTracker objects, each representing a registered task.
		/// </summary>
		Platform::TaskTracker* TaskList;

	protected:
		/// <summary>
		/// Number of currently registered tasks.
		/// </summary>
		uint_fast8_t TaskCount = 0;

		/// <summary>
		/// Indicates if the task registry state has changed (used for idle sleep logic).
		/// </summary>
		volatile bool Hot = false;

#ifdef HARMONIC_PLATFORM_OS
	protected:
		SemaphoreHandle_t IdleSleepSemaphore;
#endif

	public:
		/// <summary>
		/// Maximum number of tasks that can be registered.
		/// </summary>
		const uint_fast8_t TaskCapacity;

	public:
		/// <summary>
		/// Constructs the registry with a specified task capacity.
		/// </summary>
		/// <param name="taskCapacity">Maximum number of tasks supported.</param>
		TaskRegistry(Platform::TaskTracker* taskList, const task_id_t taskCapacity)
			: TaskList(taskList)
			, TaskCapacity(taskCapacity)
		{
#ifdef HARMONIC_PLATFORM_OS
			IdleSleepSemaphore = xSemaphoreCreateBinary();
#endif
		}

		~TaskRegistry()
		{
#ifdef HARMONIC_PLATFORM_OS
			if (IdleSleepSemaphore) vSemaphoreDelete(IdleSleepSemaphore);
#endif
		}

		/// <summary>
		/// Adds a new task to the registry.
		/// Assigns a unique taskId (its index in the array).
		/// Returns false if the task is null, already exists, or capacity is exceeded.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <param name="taskId">Output: assigned task ID.</param>
		/// <param name="delay">Initial delay before first run (ms).</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True on success, false otherwise.</returns>
		bool AttachTask(ITask* task, task_id_t& taskId, const uint32_t delay = 0, const bool enabled = true)
		{
			if (task == nullptr
				|| TaskCount >= TaskCapacity
				|| TaskExists(task))
			{
				return false;
			}

			Hot = true; // Flag hot state when collection changed.

			// Task Id is the position on the list.
			taskId = TaskCount;
			TaskCount++;
			Platform::TaskTracker& newTask = TaskList[taskId];
			newTask.Task = task;
			newTask.Delay = delay;
			newTask.Enabled = enabled;
			newTask.LastRun = Platform::GetTimestamp();

			WakeFromInterrupt();
			return true;
		}

		/// <summary>
		/// Removes all tasks from the registry.
		/// </summary>
		void Clear()
		{
			Hot = true; // Flag hot state when collection changed.
			TaskCount = 0;
		}

		/// <summary>
		/// Retrieves the taskId for a given task pointer, if it exists.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <param name="taskId">Output: found task ID.</param>
		/// <returns>True if found, false otherwise.</returns>
		bool GetTaskId(const ITask* task, task_id_t& taskId) const
		{
			for (task_id_t i = 0; i < TaskCount; i++)
			{
				if (TaskList[i].Task == task)
				{
					taskId = i;
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Checks if a given task pointer is already registered.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <returns>True if the task exists, false otherwise.</returns>
		bool TaskExists(const ITask* task) const
		{
			for (task_id_t i = 0; i < TaskCount; i++)
			{
				if (TaskList[i].Task == task)
				{
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Sets the run delay period for a task.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="delay">New delay period in milliseconds.</param>
		void SetDelay(const uint8_t taskId, const uint32_t delay)
		{
			TaskList[taskId].SetDelay(delay);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets the enabled/disabled state for a task.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="enabled">New enabled state.</param>
		void SetEnabled(const uint8_t taskId, const bool enabled)
		{
			TaskList[taskId].SetEnabled(enabled);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets both the run delay period and enabled state for a task.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="delay">New delay period in milliseconds.</param>
		/// <param name="enabled">New enabled state.</param>
		void SetDelayEnabled(const uint8_t taskId, const uint32_t delay, const bool enabled)
		{
			TaskList[taskId].SetDelayEnabled(delay, enabled);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediatelly.
		/// This method is safe to call from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		void WakeFromISR(const uint8_t taskId)
		{
			TaskList[taskId].Delay = 0;
			TaskList[taskId].Enabled = true;
			Hot = true; // Flag hot state when task state changed.
			WakeFromInterrupt();
		}

	private:
#ifdef HARMONIC_PLATFORM_OS
		/// <summary>
		/// Wakes the scheduler from idle sleep when a task is added or its state changes.
		///
		/// On RTOS platforms, this signals the scheduler's
		/// semaphore from an interrupt context; on non-RTOS platforms, it does nothing.
		/// </summary>
		void WakeFromInterrupt()
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xSemaphoreGiveFromISR(IdleSleepSemaphore, &xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
#else
		/// <summary>
		/// No-op function, compiled away.
		/// </summary>
		void WakeFromInterrupt() {}
#endif
	};
}
#endif