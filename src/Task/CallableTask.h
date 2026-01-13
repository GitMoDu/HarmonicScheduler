#ifndef _HARMONIC_CALLABLE_TASK_H
#define _HARMONIC_CALLABLE_TASK_H

#include "ExposedDynamicTask.h"

namespace Harmonic
{
	/// <summary>
	/// A DynamicTask that wraps a callable:
	///  - A plain function pointer: void()
	///  - A function pointer with context: void(void*)
	///
	/// Notes:
	///  - No dynamic allocation, no std::function.
	///  - Avoids UB from casting between incompatible function pointer types.
	/// </summary>
	class CallableTask final : public ExposedDynamicTask
	{
	public:
		using CallableWithContext_t = void (*)(void*);
		using CallableNoContext_t = void (*)();

	private:
		CallableNoContext_t RunNoContext_ = nullptr;
		CallableWithContext_t RunWithContext_ = nullptr;
		void* Context_ = nullptr;

	public:
		/// <summary>
		/// Constructor for plain function pointer (no context).
		/// </summary>
		CallableTask(TaskRegistry& registry, CallableNoContext_t runCallable)
			: ExposedDynamicTask(registry)
			, RunNoContext_(runCallable)
		{
		}

		/// <summary>
		/// Constructor for callable with context.
		/// </summary>
		CallableTask(TaskRegistry& registry, CallableWithContext_t runCallable, void* context)
			: ExposedDynamicTask(registry)
			, RunWithContext_(runCallable)
			, Context_(context)
		{
		}

		void Run() final
		{
			if (RunWithContext_ != nullptr)
			{
				RunWithContext_(Context_);
				return;
			}

			if (RunNoContext_ != nullptr)
			{
				RunNoContext_();
				return;
			}
		}
	};
}

#endif