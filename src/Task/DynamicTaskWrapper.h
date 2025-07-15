#ifndef _HARMONIC_DYNAMIC_TASK_WRAPPER_h
#define _HARMONIC_DYNAMIC_TASK_WRAPPER_h

#include "ExposedDynamicTask.h"

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
	class DynamicTaskWrapper final : public ExposedDynamicTask
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
			: ExposedDynamicTask(registry)
			, Runner(task)
		{
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
	};
}
#endif