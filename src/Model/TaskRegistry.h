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
  /// Task handles are stable after Attach() and are resolved through an internal handle-to-slot map.
	///
	/// Callability:
	/// - Attach, Detach, Clear: Not safe to call from an ISR.
	/// - SetPeriod, SetEnabled, SetPeriodAndEnabled, WakeFromISR: Safe to call from any context, including from an ISR.
   /// - TaskExists, IsEnabled, GetPeriod: Safe to call from any context.
	/// 
	/// For fast and immediate wake, WakeFromISR is designed to be safely callable from an ISR.
	/// #define HARMONIC_SKIP_CHECKS - set flag to skip index validations for maximum performance.
  /// Should only be enabled if you are sure no invalid task handles will be used, as it skips checks for task existence and index validity.
	/// </summary>
	class TaskRegistry
	{
	private:
		/// <summary>
		/// Externally allocated array of TaskTracker objects, each representing a registered task.
		/// </summary>
		Platform::TaskTracker* TaskList;
		uint8_t* HandleToSlot;
		uint8_t* SlotToHandle;

	protected:
		/// <summary>
		/// Number of currently registered tasks.
		/// </summary>
		uint_fast8_t TaskCount = 0;
		task_id_t FreeHead = 0;
		task_id_t NextHandle = 0;

		/// <summary>
		/// Indicates if the task registry state has changed (used for idle sleep logic).
		/// </summary>
		volatile bool Hot = false;

		/// <summary>
		/// Sets whether this is a hot registry that tracks changes (Hot flag) when tasks are added or removed.
		/// </summary>
		const bool HotRegistry;

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
		TaskRegistry(Platform::TaskTracker* taskList, uint8_t* handleToSlot, uint8_t* slotToHandle, const task_id_t taskCapacity, const bool hotRegistry)
			: TaskList(taskList)
			, HandleToSlot(handleToSlot)
			, SlotToHandle(slotToHandle)
			, HotRegistry(hotRegistry)
			, TaskCapacity(taskCapacity)
		{
#ifdef HARMONIC_PLATFORM_OS
			IdleSleepSemaphore = xSemaphoreCreateBinary();
#endif
			ResetStorage();
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
		 /// Returns a stable task handle, or TASK_INVALID_ID if the task is null, already exists, or capacity is exceeded.
		 /// </summary>
		 /// <param name="task">Pointer to ITask implementation.</param>
		 /// <param name="period">Initial delay before first run (ms).</param>
		 /// <param name="enabled">Initial enabled state.</param>
		/// <returns>Stable task handle on success, TASK_INVALID_ID on failure.</returns>
		task_id_t Attach(ITask* task, const uint32_t period = 0, const bool enabled = true)
		{
			if (task == nullptr
				|| TaskCount >= TaskCapacity
				|| TaskExists(task))
			{
				return TASK_INVALID_ID;
			}

			task_id_t handle;
			if (FreeHead > 0)
			{
				handle = SlotToHandle[TaskCapacity - FreeHead];
				FreeHead--;
			}
			else
			{
				if (NextHandle >= TaskCapacity)
				{
					return TASK_INVALID_ID;
				}

				handle = NextHandle;
				NextHandle++;
			}

			const task_id_t slot = TaskCount;

			TaskList[slot].BindTask(task, period, enabled);
			HandleToSlot[handle] = slot;
			SlotToHandle[slot] = handle;

			if (HotRegistry)
				Hot = true; // Flag hot state when collection changed.

			TaskCount++;
			WakeFromInterrupt();

			return handle;
		}

		/// <summary>
	  /// Removes a task from the registry by its stable handle. Not safe to call from an ISR.
		/// </summary>
		/// <param name="taskId">Task handle to remove.</param>
		/// <returns>True if removed, false otherwise.</returns>
		bool Detach(const task_id_t taskId)
		{
			if (!ValidateTaskId(taskId))
				return false;

			const task_id_t slot = HandleToSlot[taskId];
			const task_id_t lastSlot = TaskCount - 1;

			if (slot != lastSlot)
			{
				TaskList[slot] = TaskList[lastSlot];
				const task_id_t movedHandle = SlotToHandle[lastSlot];
				HandleToSlot[movedHandle] = slot;
				SlotToHandle[slot] = movedHandle;
			}

			TaskList[lastSlot].Task = nullptr;
			TaskList[lastSlot].Enabled = false;
			TaskList[lastSlot].Period = 0;
			HandleToSlot[taskId] = TASK_INVALID_ID;
			SlotToHandle[lastSlot] = TASK_INVALID_ID;
			TaskCount--;
			SlotToHandle[TaskCapacity - (FreeHead + 1)] = taskId;
			FreeHead++;

			if (HotRegistry)
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
			for (task_id_t slot = 0; slot < TaskCount; slot++)
			{
				if (TaskList[slot].Task == task)
				{
					return Detach(SlotToHandle[slot]);
				}
			}

			return false;
		}

		/// <summary>
		/// Removes all tasks from the registry. Not safe to call from an ISR.
		/// </summary>
		void Clear()
		{
			ResetStorage();

			if (HotRegistry)
				Hot = true; // Flag hot state when collection changed.
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
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return false;
#endif

			return TaskList[HandleToSlot[taskId]].IsEnabled();
		}

		/// <summary>
		/// Returns the current delay period (in milliseconds) for the specified task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <returns>The delay period in milliseconds.</returns>
		uint32_t GetPeriod(const task_id_t taskId) const
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return UINT32_MAX;
#endif

			return TaskList[HandleToSlot[taskId]].GetPeriod();
		}

		/// <summary>
		/// Sets the run delay period for a task dynamically.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="taskId">Valid task ID.</param>
		/// <param name="delay">New delay period in milliseconds.</param>
		void SetPeriod(const task_id_t taskId, const uint32_t delay)
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[HandleToSlot[taskId]].SetPeriod(delay);

			if (HotRegistry)
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
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[HandleToSlot[taskId]].SetEnabled(enabled);

			if (HotRegistry)
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
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[HandleToSlot[taskId]].SetPeriodAndEnabled(delay, enabled);

			if (HotRegistry)
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
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateTaskId(taskId))
				return;
#endif

			TaskList[HandleToSlot[taskId]].Wake();

			if (HotRegistry)
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

		/// <summary>
	   /// Validates the given task handle and returns whether it is currently attached.
		/// </summary>
	   /// <param name="taskId">The task handle to validate.</param>
		/// <returns>true if the task handle is valid; otherwise, false.</returns>
		bool ValidateTaskId(const task_id_t taskId) const
		{
			if (taskId == TASK_INVALID_ID)
			{
				// Invalid task handle: unregistered.
				return false;
			}
			else if (taskId >= NextHandle)
			{
				// Invalid task handle: unknown.
				return false;
			}
			else if (HandleToSlot[taskId] == TASK_INVALID_ID)
			{
				// Invalid task handle: detached.
				return false;
			}

			return true;
		}

		void ResetStorage()
		{
			for (task_id_t i = 0; i < TaskCapacity; i++)
			{
				TaskList[i].Task = nullptr;
				TaskList[i].Period = 0;
				TaskList[i].LastRun = 0;
				TaskList[i].Enabled = false;
				HandleToSlot[i] = TASK_INVALID_ID;
				SlotToHandle[i] = TASK_INVALID_ID;
			}

			TaskCount = 0;
			FreeHead = 0;
			NextHandle = 0;
			Hot = false;
		}
	};
}
#endif