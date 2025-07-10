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
			static_assert((interrupt_count_t)-1 > 0, "interrupt_count_t must be unsigned");
			static constexpr interrupt_count_t MaxValue = (interrupt_count_t)~(interrupt_count_t)0;

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

					Platform::AtomicGuard guard;
					InterruptCount = 0;
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

				if (interruptCount > 0 && Listener != nullptr)
				{
					Listener->OnEventInterrupt(timestamp, interruptCount);
				}

				const bool interruptPending = InterruptCount > 0;

				SetEnabled(interruptPending);
			}

			void OnInterrupt()
			{
				if (InterruptCount == 0)
				{
					{
						Platform::AtomicGuard guard;
						InterruptTimestamp = TimestampSource::Get();
						InterruptCount = InterruptCount + 1;
					}
					WakeFromISR();
				}
				else if (InterruptCount != MaxValue)
				{
					Platform::AtomicGuard guard;
					InterruptCount = InterruptCount + 1;
				}
			}
		};
	}
}
#endif