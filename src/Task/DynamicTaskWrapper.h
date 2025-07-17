#ifndef _HARMONIC_DYNAMIC_TASK_WRAPPER_h
#define _HARMONIC_DYNAMIC_TASK_WRAPPER_h

#include "ExposedDynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// DynamicTaskWrapper provides a composition interface (ITaskRun) for scheduled run callbacks .
	/// The wrapper is managed by the scheduler; user logic is decoupled from registry and task ID concerns.
	/// The callback can be swapped at any time.
	/// </summary>
	class DynamicTaskWrapper final : public ExposedDynamicTask
	{
	public:
		/// <summary>
		/// Minimal interface for user logic to be executed by the wrapper.
		/// Does not expose registry, task ID, or scheduling APIs.
		/// </summary>
		struct ITaskRun
		{
			/// <summary>
			/// Task execution callback.
			/// The method should return quickly and must not block.
			/// </summary>
			virtual void Run() = 0;
		};

	private:
		/// <summary>
		/// Pointer to the external ITaskRun to be executed.
		/// If nullptr, Run() does nothing.
		/// Can be changed at runtime for flexible composition.
		/// </summary>
		ITaskRun* Runner;

	public:
		/// <summary>
		/// Constructs a DynamicTaskWrapper.
		/// The wrapper is registered and managed by the scheduler; the user logic is only responsible for execution.
		/// </summary>
		/// <param name="registry">Reference to the TaskRegistry for scheduling and management.</param>
		/// <param name="task">Optional pointer to the ITaskRun to delegate execution to.</param>
		DynamicTaskWrapper(TaskRegistry& registry, ITaskRun* task = nullptr)
			: ExposedDynamicTask(registry)
			, Runner(task)
		{
		}

		/// <summary>
		/// Sets or replaces the underlying ITaskRun to be executed.
		/// Can be called at any time to swap the ITaskRun runner.
		/// </summary>
		/// <param name="task">Pointer to the new ITaskRun. Can be nullptr to disable execution.</param>
		void SetTaskRunner(ITaskRun* task)
		{
			Runner = task;
		}

		/// <summary>
		/// Executes the wrapped ITaskRun's Run() method if Runner is set.
		/// Overrides ExposedDynamicTask::Run().
		/// </summary>
		void Run() final
		{
			if (Runner != nullptr)
			{
				Runner->Run();
			}
		}
	};
}
#endif