#ifndef _HARMONIC_CALLABLE_TASK_H
#define _HARMONIC_CALLABLE_TASK_H

#include "DynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// A DynamicTask that wraps a callable (function pointer or lambda/functor with optional context pointer).
	/// - Pass a function pointer (optionally with a context pointer) to the constructor.
	/// - The callable will be invoked with the context pointer on each Run().
	/// - No dynamic allocation or std::function.
	/// </summary>
	class CallableTask final : public DynamicTask
	{
	public:
		typedef void (*Callable_t)(void*);

	private:
		Callable_t RunCallable;
		void* Context;

	public:
		// Constructor for plain function pointer (no context)
		CallableTask(TaskRegistry& registry, void (*runCallable)())
			: DynamicTask(registry)
			, RunCallable(reinterpret_cast<Callable_t>(runCallable))
			, Context(nullptr)
		{
			// Only pointer size is checked for compatibility due to C++14 limitations.
			static_assert(sizeof(runCallable) == sizeof(Callable_t),
				"CallableTask: Function pointer size mismatch. This platform may not support casting between these types.");
		}

		// Constructor for callable with context (e.g., lambda with capture)
		CallableTask(TaskRegistry& registry, Callable_t runCallable, void* context)
			: DynamicTask(registry)
			, RunCallable(runCallable)
			, Context(context)
		{
		}

		void Run() final
		{
			if (RunCallable)
			{
				RunCallable(Context);
			}
		}
	
	public:
		/// <summary>
		/// Returns the unique task ID assigned by the registry.
		/// </summary>
		/// <returns>Task ID, or UINT8_MAX if not registered.</returns>
		task_id_t GetTaskId() const
		{
			return DynamicTask::GetTaskId();
		}

		/// <summary>
		/// Returns true if this task is currently enabled in the registry.
		/// </summary>
		bool IsEnabled() const
		{
			return DynamicTask::IsEnabled();
		}

		/// <summary>
		/// Returns the current delay (period) for this task in milliseconds.
		/// </summary>
		uint32_t GetDelay() const
		{
			return DynamicTask::GetDelay();
		}

		/// <summary>
		/// Sets the execution period (delay) for this task.
		/// </summary>
		/// <param name="delay">New execution period in milliseconds.</param>
		void SetDelay(const uint32_t delay)
		{
			DynamicTask::SetDelay(delay);
		}

		/// <summary>
		/// Enables or disables this task in the registry.
		/// </summary>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetEnabled(const bool enabled)
		{
			DynamicTask::SetEnabled(enabled);
		}

		/// <summary>
		/// Sets both the execution period and enabled state for this task.
		/// </summary>
		/// <param name="delay">New execution period in milliseconds.</param>
		/// <param name="enabled">True to enable, false to disable.</param>
		void SetDelayEnabled(const uint32_t delay, const bool enabled)
		{
			DynamicTask::SetDelayEnabled(delay, enabled);
		}

		/// <summary>
		/// Wakes the scheduler and sets the task to run immediately.
		/// Safe to call from an ISR.
		/// </summary>
		void WakeFromISR()
		{
			DynamicTask::WakeFromISR();
		}
	};
}

#endif