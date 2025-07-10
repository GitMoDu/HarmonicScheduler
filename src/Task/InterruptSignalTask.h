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

					return true;
				}

				return false;
			}

		public:
			void Run() final
			{
				signal_t signal;
				{
					signal_t signal;
#if !defined(UINTPTR_MAX) || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
					// On platforms with pointer size < 32 bits (usually 8-bit MCUs), guard for types > 8 bits
					if (sizeof(signal_t) > 1)
					{
						{
							Platform::AtomicGuard guard;
							signal = InterruptSignal;
						}
					}
					else
					{
						signal = InterruptSignal;
					}
#else
					// On 32-bit+ platforms, guard only for types > 4 bytes
					if (sizeof(signal_t) > 4)
					{
						{
							Platform::AtomicGuard guard;
							signal = InterruptSignal;
						}
					}
					else
					{
						signal = InterruptSignal;
					}
#endif
					InterruptSignal = 0;
				}

				if (Listener != nullptr)
				{
					Listener->OnSignalInterrupt(signal);
				}

				const bool interruptPending = InterruptSignal > 0;

				SetEnabled(interruptPending);
			}

			void OnInterrupt()
			{
#if !defined(UINTPTR_MAX) || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				// On 8-bit platforms, guard for types > 8 bits
				if (sizeof(signal_t) > 1)
				{
					{
						Platform::AtomicGuard guard;
						InterruptSignal++;
					}
				}
				else
				{
					InterruptSignal++;
				}
#else
				// On 32-bit+ platforms, guard only for types > 4 bytes
				if (sizeof(signal_t) > 4)
				{
					{
						Platform::AtomicGuard guard;
						InterruptSignal++;
					}
				}
				else
				{
					InterruptSignal++;
				}
#endif
				WakeFromISR();
			}
		};

	}
}
#endif