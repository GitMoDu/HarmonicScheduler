#ifndef _HARMONIC_CALLABLE_TASK_H
#define _HARMONIC_CALLABLE_TASK_H

#include "ExposedDynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// A DynamicTask that wraps a callable (function pointer or lambda/functor with optional context pointer).
	/// - Pass a function pointer (optionally with a context pointer) to the constructor.
	/// - The callable will be invoked with the context pointer on each Run().
	/// - No dynamic allocation or std::function.
	/// </summary>
	class CallableTask final : public ExposedDynamicTask
	{
	public:
		typedef void (*Callable_t)(void*);

	private:
		Callable_t RunCallable;
		void* Context;

	public:
		// Constructor for plain function pointer (no context)
		CallableTask(TaskRegistry& registry, void (*runCallable)())
			: ExposedDynamicTask(registry)
			, RunCallable(reinterpret_cast<Callable_t>(runCallable))
			, Context(nullptr)
		{
			// Only pointer size is checked for compatibility due to C++14 limitations.
			static_assert(sizeof(runCallable) == sizeof(Callable_t),
				"CallableTask: Function pointer size mismatch. This platform may not support casting between these types.");
		}

		// Constructor for callable with context (e.g., lambda with capture)
		CallableTask(TaskRegistry& registry, Callable_t runCallable, void* context)
			: ExposedDynamicTask(registry)
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
	};
}

#endif