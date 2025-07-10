#ifndef _HARMONIC_INTERRUPT_SIGNAL_TASK_h
#define _HARMONIC_INTERRUPT_SIGNAL_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptSignal
	{
		template<typename signal_t = uint8_t>
		struct InterruptListener
		{
			virtual void OnSignalInterrupt(const signal_t signalCount) = 0;
		};

		template<typename signal_t = uint8_t>
		class CallbackTask final : public DynamicTask
		{
		private:
			static_assert((signal_t)-1 > 0, "signal_t must be unsigned");
			static constexpr signal_t MaxValue = (signal_t)~(signal_t)0;

		private:
			volatile signal_t InterruptSignal = 0;

		private:
			InterruptListener<signal_t>* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			bool AttachListener(InterruptListener<signal_t>* listener)
			{
				if (Attach(0, false))
				{
					Listener = listener;

					Platform::AtomicGuard guard;
					InterruptSignal = 0;
					return true;
				}

				return false;
			}

		public:
			void Run() final
			{
				signal_t signal;
				{
					Platform::AtomicGuard guard;
					signal = InterruptSignal;
					InterruptSignal = 0;
				}

				if (signal > 0 && Listener != nullptr)
				{
					Listener->OnSignalInterrupt(signal);
				}

				const bool interruptPending = InterruptSignal > 0;

				SetEnabled(interruptPending);
			}

			void OnInterrupt()
			{
				{
					Platform::AtomicGuard guard;
					if (InterruptSignal != MaxValue)
						InterruptSignal = InterruptSignal + 1;
				}
				WakeFromISR();
			}
		};
	}
}
#endif