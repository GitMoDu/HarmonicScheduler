#ifndef _HARMONIC_INTERRUPT_FLAG_TASK_h
#define _HARMONIC_INTERRUPT_FLAG_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptFlag
	{
		struct InterruptListener
		{
			virtual void OnFlagInterrupt() = 0;
		};

		class CallbackTask final : public DynamicTask
		{
		private:
			volatile bool InterruptFlag = false;

		private:
			InterruptListener* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			bool AttachListener(InterruptListener* listener)
			{
				if (Attach(0, false))
				{
					Listener = listener;

					Platform::AtomicGuard guard;
					InterruptFlag = false;
					return true;
				}

				return false;
			}

		public:
			void Run() final
			{
				bool flag;
				{
					Platform::AtomicGuard guard;
					flag = InterruptFlag;
					InterruptFlag = false;
				}

				if (flag && Listener != nullptr)
				{
					Listener->OnFlagInterrupt();
				}

				const bool interruptPending = InterruptFlag > 0;

				SetEnabled(interruptPending);
			}

			void OnInterrupt()
			{
				if (!InterruptFlag)
				{
					InterruptFlag = true;
					WakeFromISR();
				}
			}
		};
	}
}
#endif