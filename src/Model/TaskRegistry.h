#ifndef _HARMONIC_TASK_REGISTRY_h
#define _HARMONIC_TASK_REGISTRY_h

#include "ITask.h"
#include "TaskTracker.h"
#include "../Platform/Platform.h"
#include "../Platform/Atomic.h"
#include "../Platform/Timestamp.h"
#include "../Platform/IdleSleep.h"

namespace Harmonic
{
	/// <summary>
	/// TaskRegistry provides dynamic registration, removal, and management of cooperative tasks.
	/// 
	/// Stores pointers to ITask implementations in a externally allocated array of TaskTracker objects.
	/// Supports adding, removing, clearing, and querying tasks, as well as updating their delay and enabled state.
	/// Task handles remain stable for the duration of an attachment and are
	/// resolved through an internal handle-to-slot map. They are not
	/// lifetime-unique identifiers: a value may be recycled after Detach() or
	/// Clear(), so callers must not retain handles for removed tasks.
	///
	/// Callability:
	/// - Attach, Detach, Clear: Not safe to call from an ISR.
	/// - All other methods are safe to call from any context, including from an ISR.
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
		task_handle_t* HandleToSlot;

	protected:
		/// <summary>
		/// Number of currently registered tasks.
		/// </summary>
		task_index_t TaskCount = 0;

		/// <summary>
		/// Per-scheduler-iteration activity flag.
		///
		/// Scheduler implementations clear this before dispatching tasks. A task
		/// execution or a registry mutation may set it, allowing idle-sleep
		/// schedulers to distinguish an idle iteration from an active one.
		/// This flag is not required for normal task dispatch.
		/// </summary>
		volatile bool Hot = false;

		/// <summary>
		/// Called after the registered task collection changes.
		/// </summary>
		virtual void OnTaskCollectionChanged()
		{}

		/// <summary>
		/// Enables registry-mutation tracking through the Hot activity flag.
		///
		/// Idle-sleep scheduler variants pass true so attach, detach, clear, and
		/// other registry changes keep the current iteration awake. Non-sleeping
		/// variants pass false because they do not inspect the Hot flag.
		/// </summary>
		const bool HotRegistry;

#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
	protected:
		SemaphoreHandle_t IdleSleepSemaphore;
#endif

	public:
		/// <summary>
		/// Maximum number of tasks that can be registered.
		/// </summary>
		const task_index_t TaskCapacity;

	public:
		/// <summary>
		/// Constructs the registry with a specified task capacity and optional
		/// activity tracking for idle-sleep schedulers.
		/// </summary>
		/// <param name="taskCapacity">Maximum number of tasks supported.</param>
		/// <param name="hotRegistry">True when the owning scheduler uses Hot to decide whether to idle sleep.</param>
		TaskRegistry(Platform::TaskTracker* taskList, task_handle_t* handleToSlot, const task_index_t taskCapacity, const bool hotRegistry)
			: TaskList(taskList)
			, HandleToSlot(handleToSlot)
			, HotRegistry(hotRegistry)
			, TaskCapacity(taskCapacity)
		{
#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
			IdleSleepSemaphore = xSemaphoreCreateBinary();
#endif
			ResetStorage();
		}

		virtual ~TaskRegistry()
		{
#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
			if (IdleSleepSemaphore) vSemaphoreDelete(IdleSleepSemaphore);
#endif
		}

		/// <summary>
		/// Gets the number of currently registered tasks.
		/// </summary>
		/// <returns>The number of registered tasks.</returns>
		task_index_t GetTaskCount() const
		{
			return TaskCount;
		}

		/// <summary>
		/// Adds a new task to the registry. Not safe to call from an ISR.
		/// Returns a handle that remains stable until this attachment is removed,
		/// or TASK_INVALID_HANDLE if the task is null, already exists, or capacity
		/// is exceeded. The returned value is a registry-local reference, not a
		/// lifetime-unique task identifier.
		/// If a task is already registered, its delay and enabled state are updated and the existing handle is returned.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <param name="delay">Initial execution delay for the task, in milliseconds.</param>
		/// <param name="enabled">Initial enabled state.</param>
		/// <returns>Attachment-stable handle on success, TASK_INVALID_HANDLE on failure.</returns>
		task_handle_t Attach(ITask* task, const uint32_t delay = 0, const bool enabled = true)
		{
			// All task callbacks must be non-null to be valid.
			if (task == nullptr)
			{
				return TASK_INVALID_HANDLE;
			}

			// Find if the task is already registered by searching for its pointer in the current list.
			task_handle_t handle = TASK_INVALID_HANDLE;
			for (task_index_t i = 0; i < TaskCount; i++)
			{
				if (TaskList[i].Task == task)
				{
					handle = TaskList[i].Handle;
					break;
				}
			}

			if (handle != TASK_INVALID_HANDLE)
			{
				// Task already exists, update its delay and enabled state.
				SetDelayFromNow(handle, delay);
				SetEnabled(handle, enabled);

				return handle;
			}
			else
			{
				// If a new slot is required and the registry is at capacity, return an invalid handle.
				if (TaskCount >= TaskCapacity)
				{
					return TASK_INVALID_HANDLE;
				}

				// Allocate a handle that remains stable for this attachment by scanning
				// from the current task count, wrapping once if necessary. This avoids
				// storing a separate handle cursor while preserving O(1) handle-to-task
				// lookup during execution.
				for (task_handle_t offset = 0; offset < TaskCapacity; offset++)
				{
					const task_handle_t candidate = (TaskCount + offset < TaskCapacity)
						? TaskCount + offset
						: TaskCount + offset - TaskCapacity;
					if (HandleToSlot[candidate] == TASK_INVALID_HANDLE)
					{
						handle = candidate;
						break;
					}
				}

				// No handle found, registry is full (all handles in use).
				if (handle == TASK_INVALID_HANDLE)
				{
					return TASK_INVALID_HANDLE;
				}

				const task_handle_t slot = TaskCount;

				// Place the new task into the dense tail of TaskList. By using the
				// TaskCount index we keep TaskList compact for efficient iteration
				// by the scheduler (no holes in the active range [0, TaskCount)).
				TaskList[slot].BindTask(task, delay, handle, enabled);

				// Record the handle-to-slot mapping. The scheduler keeps a dense task
				// array while callers retain the handle as the stable reference.
				HandleToSlot[handle] = slot;

				if (HotRegistry)
					Hot = true; // Flag hot state when collection changed.

				// Increase the count of active tasks and wake the scheduler so it
				// re-evaluates scheduling with the newly attached task.
				TaskCount++;
				OnTaskCollectionChanged();

				// Force the scheduler to wake up immediately to consider the new task.
				WakeFromInterrupt();

				return handle;
			}
		}

		/// <summary>
		/// Removes a task from the registry by its current attachment handle.
		/// Not safe to call from an ISR - Attach/Detach/Clear must be invoked
		/// from the single-threaded scheduler/main context. A brief internal
		/// AtomicGuard is used only to prevent ISRs from seeing transient state
		/// while the registry compacts the dense task list.
		/// </summary>
		/// <param name="handle">Task handle to remove.</param>
		void Detach(const task_handle_t handle)
		{
			// Quick parameter validation first.
			if (
#if !defined(HARMONIC_SKIP_CHECKS)
				handle == TASK_INVALID_HANDLE
				|| handle >= TaskCapacity
				||
#endif
				TaskCount == 0)
			{
				return;
			}

			// Find the slot currently assigned to this handle.
			const task_handle_t slot = HandleToSlot[handle];
			if (slot == TASK_INVALID_HANDLE || slot >= TaskCount)
			{
				return;
			}

			const task_handle_t lastSlot = TaskCount - 1;
			const task_handle_t movedHandle = (slot != lastSlot)
				? TaskList[lastSlot].Handle
				: TASK_INVALID_HANDLE;

			// Invalidate affected handles before changing tracker storage.
			// This prevents ISRs from touching either tracker while the dense
			// list is compacted below.
			{
				Platform::AtomicGuard guard;

				HandleToSlot[handle] = TASK_INVALID_HANDLE;

				if (movedHandle != TASK_INVALID_HANDLE)
					HandleToSlot[movedHandle] = TASK_INVALID_HANDLE;
			}

			// Move the last entry into the removed slot to avoid leaving holes
			// in TaskList. No valid handle can reach either affected slot here.
			if (movedHandle != TASK_INVALID_HANDLE)
				TaskList[slot] = TaskList[lastSlot];

			// Clear the old last slot which we've either moved out or are removing.
			TaskList[lastSlot].Unbind();

			// Publish moved-handle mapping only after tracker data is complete.
			if (movedHandle != TASK_INVALID_HANDLE)
			{
				Platform::AtomicGuard guard;
				HandleToSlot[movedHandle] = slot;
			}

			TaskCount--;

			if (HotRegistry)
				Hot = true; // Flag hot state when collection changed.

			OnTaskCollectionChanged();
		}

		/// <summary>
		/// Removes a task from the registry by its pointer. Not safe to call from an ISR.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		void Detach(const ITask* task)
		{
			for (task_index_t slot = 0; slot < TaskCount; slot++)
			{
				if (TaskList[slot].Task == task)
				{
					Detach(TaskList[slot].Handle);
					return;
				}
			}
		}

		/// <summary>
		/// Removes all tasks from the registry. Not safe to call from an ISR.
		/// </summary>
		void Clear()
		{
			ResetStorage();

			if (HotRegistry)
				Hot = true; // Flag hot state when collection changed.

			OnTaskCollectionChanged();
		}

		/// <summary>
		/// Checks if a given task pointer is already registered.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="task">Pointer to ITask implementation.</param>
		/// <returns>True if the task exists, false otherwise.</returns>
		bool TaskExists(const ITask* task) const
		{
			for (task_index_t i = 0; i < TaskCount; i++)
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
		/// <param name="handle">Valid task handle.</param>
		/// <returns>True if the task is enabled, false otherwise.</returns>
		bool IsEnabled(const task_handle_t handle) const
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return false;
#endif

			return TaskList[HandleToSlot[handle]].IsEnabled();
		}

		/// <summary>
		/// Returns the current delay (in milliseconds) for the specified task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="handle">Valid task handle.</param>
		/// <returns>The delay in milliseconds.</returns>
		uint32_t GetDelay(const task_handle_t handle) const
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return UINT32_MAX;
#endif

			return TaskList[HandleToSlot[handle]].GetDelay();
		}

		/// <summary>
		/// Sets the enabled/disabled state for a task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="handle">Valid task handle.</param>
		/// <param name="enabled">New enabled state.</param>
		void SetEnabled(const task_handle_t handle, const bool enabled)
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return;
#endif

			TaskList[HandleToSlot[handle]].SetEnabled(enabled);

			if (HotRegistry)
				Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets the run delay for a task, from the last run timestamp.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="handle">Valid task handle.</param>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelay(const task_handle_t handle, const uint32_t delay)
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return;
#endif
			TaskList[HandleToSlot[handle]].SetDelay(delay);

			if (HotRegistry)
				Hot = true; // Flag hot state when task state changed.
		}

		/// <summary>
		/// Sets the run delay for a task, relative to the current time.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="handle">Valid task handle.</param>
		/// <param name="delay">New delay in milliseconds.</param>
		void SetDelayFromNow(const task_handle_t handle, const uint32_t delay)
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return;
#endif
			if (HotRegistry)
				Hot = true; // Flag hot state when task state changed.

			TaskList[HandleToSlot[handle]].SetDelayFromNow(delay);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Best way to quickly wake a task.
		/// Safe to call from any context, including from an ISR.
		/// </summary>
		/// <param name="handle">Valid task handle.</param>
		void WakeFromISR(const task_handle_t handle)
		{
#if !defined(HARMONIC_SKIP_CHECKS)
			if (!ValidateHandle(handle))
				return;
#endif

			TaskList[HandleToSlot[handle]].Wake();

			if (HotRegistry)
				Hot = true; // Flag hot state when task state changed.

			WakeFromInterrupt();
		}

	private:
#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
		/// <summary>
		/// Wakes the scheduler from idle sleep when a task is added or its state changes.
		///
		/// On RTOS and hosted OS platforms, this signals the scheduler semaphore;
		/// on other platforms, it does nothing.
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
		/// <param name="handle">The task handle to validate.</param>
		/// <returns>true if the task handle is valid; otherwise, false.</returns>
		bool ValidateHandle(const task_handle_t handle) const
		{
			if (handle == TASK_INVALID_HANDLE)
			{
				// Invalid task handle: unregistered.
				return false;
			}
			else if (handle >= TaskCapacity)
			{
				// Invalid task handle: unknown.
				return false;
			}
			else if (HandleToSlot[handle] == TASK_INVALID_HANDLE)
			{
				// Invalid task handle: detached.
				return false;
			}

			return true;
		}

		void ResetStorage()
		{
			for (task_index_t i = 0; i < TaskCapacity; i++)
			{
				TaskList[i].Task = nullptr;
				TaskList[i].Handle = TASK_INVALID_HANDLE;
				TaskList[i].Delay = 0;
				TaskList[i].LastRun = 0;
				TaskList[i].Enabled = false;
				HandleToSlot[i] = TASK_INVALID_HANDLE;
			}

			TaskCount = 0;
			Hot = false;
		}

	};
}
#endif