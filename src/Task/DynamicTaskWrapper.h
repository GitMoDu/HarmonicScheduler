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
		DynamicTaskWrapper(IScheduler& scheduler, ITask* runner)
			: DynamicTask(scheduler)
			, Runner(runner)
		{
		}

	public:
		/// <summary>
		/// Execute callback wrapper.
		/// </summary>
		virtual void Run() final
		{
			Runner->Run();
		}
	};
}
#endif