#ifndef _HARMONIC_INTERRUPT_EVENT_TASK_h
#define _HARMONIC_INTERRUPT_EVENT_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptEvent
	{
		template<typename interrupt_count_t = uint8_t>
		struct InterruptListener
		{
			virtual void OnEventInterrupt(const uint32_t timestamp, const interrupt_count_t interruptions) = 0;
		};

		struct MicrosTimestampSource
		{
			static uint32_t Get()
			{
				return micros();
			}
		};

		struct MillisTimestampSource
		{
			static uint32_t Get()
			{
				return millis();
			}
		};

		template<typename TimestampSource = MicrosTimestampSource,
			typename interrupt_count_t = uint8_t>
		class CallbackTask final : public DynamicTask
		{
		private:
			volatile uint32_t InterruptTimestamp = 0;
			volatile interrupt_count_t InterruptCount = 0;

		private:
			InterruptListener<interrupt_count_t>* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			bool AttachListener(InterruptListener<interrupt_count_t>* listener)
			{
				if (Attach(0, false))
				{
					Listener = listener;

					return true;
				}

				return false;
			}

			void Run() final
			{
				uint32_t timestamp;
				interrupt_count_t interruptCount;
				{
					Platform::AtomicGuard guard;
					timestamp = InterruptTimestamp;
					interruptCount = InterruptCount;
					InterruptCount = 0;
				}

				if (Listener != nullptr)
				{
					Listener->OnEventInterrupt(timestamp, interruptCount);
				}

				const bool interruptPending = InterruptCount > 0;

				SetEnabled(interruptPending);
			}

			void OnInterrupt()
			{
#if !defined(UINTPTR_MAX) || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
				if (InterruptCount == 0)
				{
					{
						Platform::AtomicGuard guard;
						InterruptTimestamp = TimestampSource::Get();
						InterruptCount++;
					}
					WakeFromISR();
				}
				else
				{
					// On 8-bit platforms, guard for types > 8 bits
					if (sizeof(interrupt_count_t) > 1)
						Platform::AtomicGuard guard;
					InterruptCount++;
				}
#else
				if (InterruptCount == 0)
				{
					{
						Platform::AtomicGuard guard;
						InterruptTimestamp = TimestampSource::Get();
						InterruptCount++;
					}
					WakeFromISR();
				}
				else
				{
					// On 32-bit+ platforms, guard only for types > 4 bytes
					if (sizeof(interrupt_count_t) > 4)
						Platform::AtomicGuard guard;
					InterruptCount++;
				}
#endif
			}
		};
	}
}
#endif