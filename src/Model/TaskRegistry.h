#ifndef _HARMONIC_TASK_REGISTRY_h
#define _HARMONIC_TASK_REGISTRY_h

#include "ITask.h"
#include "TaskTracker.h"
#include "../Platform/Platform.h"
#include "../Platform/Timestamp.h"
#include "../Platform/IdleSleep.h"

namespace Harmonic
{
	/// <summary>
	/// TaskRegistry provides dynamic registration, removal, and management of cooperative tasks.
	/// 
	/// Stores pointers to ITask implementations in a externally allocated array of TaskTracker objects.
	/// Supports adding, removing, clearing, and querying tasks, as well as updating their delay and enabled state.
	/// Task IDs are assigned and updated dynamically; tasks are notified of their current ID via OnTaskIdUpdated.
	///
	/// Callability:
	/// - Attach, Detach, Clear: Not safe to call from an ISR.
	/// - SetPeriod, SetEnabled, SetPeriodAndEnabled, WakeFromISR: Safe to call from any context, including from an ISR.
	/// - GetTaskId, TaskExists, IsEnabled, GetPeriod: Safe to call from any context.
	/// 
	/// For fast and immediate wake, WakeFromISR is designed to be safely callable from an ISR.
	/// Set flag #define HARMONIC_OPTIMIZATIONS to skip index validation for task management and faster wakeups.
	/// Should only be enabled if you are sure no invalid task IDs will be used, as it skips checks for task existence and index validity.
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

		task_id_t GetTaskCount() const
		{
			return TaskCount;
		}

		/// <summary>
		/// Adds a new task to the registry. Not safe to call from an ISR.
		/// Assigns it a unique task ID (its index in the array) and notifies the task via OnTaskIdUpdated.
		/// Returns false if the task is null, already exists, or capacity is exceeded.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <param name="period">Initial delay before first run (ms).</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>True on success, false otherwise.</returns>
		bool Attach(ITask* task, const uint32_t period = 0, const bool enabled = true)
		{
			if (task == nullptr
				|| TaskCount >= TaskCapacity
				|| TaskExists(task))
			{
				return false;
			}

			// The task ID is the next available index in the TaskList.
			const task_id_t taskId = TaskCount;

			// Bind Task at the position on the list (task ID).
			TaskList[taskId].BindTask(task, period, enabled);

			// Notify the task of its assigned ID.
			TaskList[taskId].NotifyTaskIdUpdate(taskId);

			Hot = true; // Flag hot state when collection changed.
			TaskCount++;
			WakeFromInterrupt();

			return true;
		}

		/// <summary>
		/// Removes a task from the registry by its task ID. Not safe to call from an ISR.
		/// Shifts remaining tasks to fill the gap and updates their IDs via OnTaskIdUpdated.
		/// The removed task is notified with TASK_INVALID_ID.
		/// </summary>
		/// <param name="taskId">Task ID to remove.</param>
		/// <returns>True if removed, false otherwise.</returns>
		bool Detach(const task_id_t taskId)
		{
			if (taskId >= TaskCount)
				return false;

			// Notify the removed task.
			TaskList[taskId].NotifyTaskIdUpdate(TASK_INVALID_ID);

			// Shift all tasks after the removed one to fill the gap.
			for (task_id_t i = taskId; i < TaskCount - 1; i++)
			{
				TaskList[i] = TaskList[i + 1];
				TaskList[i].NotifyTaskIdUpdate(i); // Update task ID in the moved task.
			}
			TaskCount--;
			Hot = true; // Flag hot state when collection changed.

			return true;
		}

		/// <summary>
		/// Removes a task from the registry by its pointer. Not safe to call from an ISR.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <returns>True if removed, false otherwise.</returns>
		bool Detach(const ITask* task)
		{
			task_id_t taskId;
			if (!GetTaskId(task, taskId))
			{
				return false; // Task not found.
			}

			// Find the task ID and delegate to Detach(taskId).
			return Detach(taskId);
		}

		/// <summary>
		/// Removes all tasks from the registry. Not safe to call from an ISR.
		/// Notifies each task of removal via OnTaskIdUpdated(TASK_INVALID_ID).
		/// </summary>
		void Clear()
		{
			for (task_id_t i = 0; i < TaskCount; i++)
			{
				TaskList[i].NotifyTaskIdUpdate(TASK_INVALID_ID); // Update task ID in the removed task.
			}
			Hot = true; // Flag hot state when collection changed.
			TaskCount = 0;
		}

		/// <summary>
		/// Retrieves the task ID for a given task pointer, if it exists.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <param name="taskId">Output: found task ID.</param>
		/// <returns>True if found, false otherwise.</returns>
		bool GetTaskId(const ITask* task, task_id_t& taskId) const
		{
			taskId = TASK_INVALID_ID;
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
		/// Safe to call from any context, including from an ISR.
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
		/// Returns whether the specified task is currently enabled.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <returns>True if the task is enabled, false otherwise.</returns>
		bool IsEnabled(const task_id_t taskId) const
		{
			if (taskId >= TaskCount)
				return false;

			return TaskList[taskId].IsEnabled();
		}

		/// <summary>
		/// Returns the current delay period (in milliseconds) for the specified task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <returns>The delay period in milliseconds.</returns>
		uint32_t GetPeriod(const task_id_t taskId) const
		{
			if (taskId >= TaskCount)
				return UINT32_MAX;

			return TaskList[taskId].GetPeriod();
		}

		/// <summary>
		/// Sets the run delay period for a task dynamically.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="delay">New delay period in milliseconds.</param>
		void SetPeriod(const task_id_t taskId, const uint32_t delay)
		{
#if !defined(HARMONIC_OPTIMIZATIONS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[taskId].SetPeriod(delay);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets the enabled/disabled state for a task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="enabled">New enabled state.</param>
		void SetEnabled(const task_id_t taskId, const bool enabled)
		{
#if !defined(HARMONIC_OPTIMIZATIONS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[taskId].SetEnabled(enabled);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets both the run delay period and enabled state for a task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="delay">New delay period in milliseconds.</param>
		/// <param name="enabled">New enabled state.</param>
		void SetPeriodAndEnabled(const task_id_t taskId, const uint32_t delay, const bool enabled)
		{
#if !defined(HARMONIC_OPTIMIZATIONS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[taskId].SetPeriodAndEnabled(delay, enabled);
			Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Best way to quickly wake a task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		void WakeFromISR(const task_id_t taskId)
		{
#if !defined(HARMONIC_OPTIMIZATIONS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[taskId].Period = 0;
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

#if !defined(HARMONIC_OPTIMIZATIONS)
		/// <summary>
		/// Validates the given task ID and logs an error if invalid.
		/// </summary>
		/// <param name="taskId">The task ID to validate.</param>
		/// <returns>true if the task ID is valid; otherwise, false.</returns>
		bool ValidateTaskId(const task_id_t taskId)
		{
#if defined(HARMONIC_ERROR_LOGGER)
			HARMONIC_ERROR_LOGGER.println();
			HARMONIC_ERROR_LOGGER.print(F("#Invalid Task Id: "));
#endif
			if (taskId == TASK_INVALID_ID)
			{
#if defined(HARMONIC_ERROR_LOGGER)
				HARMONIC_ERROR_LOGGER.println(F("unregistered."));
#endif
				return false;
			}
			else if (taskId >= TaskCount)
			{
#if defined(HARMONIC_ERROR_LOGGER)
				HARMONIC_ERROR_LOGGER.println(F("unknown"));
#endif
				return false;
			}

			return true;
		}
#endif
	};
}
#endif