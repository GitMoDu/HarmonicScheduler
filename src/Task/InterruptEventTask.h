#ifndef _HARMONIC_INTERRUPT_EVENT_TASK_h
#define _HARMONIC_INTERRUPT_EVENT_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptEvent
	{
		/// <summary>
		/// Interface for receiving timestamped interrupt events from CallbackTask.
		/// </summary>
		/// <typeparam name="interrupt_count_t">Type used for counting interrupts (must be unsigned, e.g., uint8_t, uint16_t).</typeparam>
		template<typename interrupt_count_t = uint8_t>
		struct InterruptListener
		{
			/// <summary>
			/// Called from main context (loop) when one or more event interrupts were triggered.
			/// </summary>
			virtual void OnEventInterrupt(const uint32_t timestamp, const interrupt_count_t interruptions) = 0;
		};

		/// <summary>
		/// Timestamp source using micros().
		/// </summary>
		struct MicrosTimestampSource
		{
			static uint32_t Get()
			{
				return micros();
			}
		};

		/// <summary>
		/// Timestamp source using millis().
		/// </summary>
		struct MillisTimestampSource
		{
			static uint32_t Get()
			{
				return millis();
			}
		};

		/// <summary>
		/// CallbackTask manages timestamped event interrupts with a count.
		///
		/// - Use OnInterrupt() from an ISR to record the timestamp and increment the event count.
		/// - The Run() method is called by the scheduler to process the event and notify the listener.
		/// - All accesses to the timestamp and count are guarded for atomicity and ISR safety.
		/// - Multiple interrupts before Run() are accumulated and reported as a count.
		/// - The event count saturates at MaxValue; further interrupts are ignored until processed.
		/// </summary>
		/// <typeparam name="TimestampSource">Type providing a static Get() method returning a timestamp (e.g., MicrosTimestampSource, MillisTimestampSource).</typeparam>
		/// <typeparam name="interrupt_count_t">Type used for counting interrupts (must be unsigned, e.g., uint8_t, uint16_t).</typeparam>
		template<typename TimestampSource = MicrosTimestampSource,
			typename interrupt_count_t = uint8_t>
		class CallbackTask final : public DynamicTask
		{
		private:
			/// <summary>
			/// Compile-time check: interrupt_count_t must be unsigned.
			/// </summary>
			static_assert((interrupt_count_t)-1 > 0, "interrupt_count_t must be unsigned");

			/// <summary>
			/// Maximum value for the event count (saturation limit).
			/// </summary>
			static constexpr interrupt_count_t MaxValue = (interrupt_count_t)~(interrupt_count_t)0;

		private:
			volatile uint32_t InterruptTimestamp = 0;
			volatile interrupt_count_t InterruptCount = 0;

		private:
			/// <summary>
			/// Listener to be notified when the event is processed.
			/// </summary>
			InterruptListener<interrupt_count_t>* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			/// <summary>
			/// Attaches an InterruptListener to receive event notifications.
			/// </summary>
			/// <param name="listener">Pointer to the listener implementation.</param>
			/// <returns>True if successfully attached, false otherwise.</returns>
			bool AttachListener(InterruptListener<interrupt_count_t>* listener)
			{
				// Registers this task with the scheduler using:
				//   delay = 0 (run immediately when triggered)
				//   enabled = false (task starts disabled until an interrupt occurs)
				if (Attach(0, false))
				{
					Listener = listener;

					Platform::AtomicGuard guard;
					InterruptCount = 0;
					return true;
				}
				return false;
			}

			/// <summary>
			/// Called by the scheduler to process the event interrupt.
			/// If the event count is nonzero, notifies the listener and clears the count and timestamp.
			/// </summary>
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

				const bool interruptPending = InterruptCount != 0;
				SetEnabled(interruptPending);
			}

			/// <summary>
			/// Called from an ISR to record the timestamp and increment the event count.
			/// If the count is at MaxValue, further interrupts are ignored until processed.
			/// </summary>
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