#ifndef _HARMONIC_DYNAMIC_TASK_WRAPPER_h
#define _HARMONIC_DYNAMIC_TASK_WRAPPER_h

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// Harmonic Task wrapper.
	/// Provides an external ITask::Run() callback for class compositing.
	/// </summary>
	class DynamicTaskWrapper : public DynamicTask
	{
	private:
		ITask* Runner;

	public:
		DynamicTaskWrapper(TaskRegistry& registry, ITask* task = nullptr)
			: DynamicTask(registry)
			, Runner(task)
		{
		}

		void SetTask(ITask* task)
		{
			Runner = task;
		}

	public:
		/// <summary>
		/// Execute callback wrapper.
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