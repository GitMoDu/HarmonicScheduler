#ifndef _HARMONIC_INTERRUPT_SIGNAL_TASK_h
#define _HARMONIC_INTERRUPT_SIGNAL_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptSignal
	{
		/// <summary>
		/// Interface for receiving signal-based interrupt events from CallbackTask.
		/// </summary>
		/// <typeparam name="signal_t">Type used for interrupt signal counting (must be unsigned, e.g., uint8_t, uint16_t).</typeparam>
		template<typename signal_t = uint8_t>
		struct InterruptListener
		{
			/// <summary>
			/// Called from main context (loop) when one or more signal interrupts were triggered.
			/// </summary>
			virtual void OnSignalInterrupt(const signal_t signalCount) = 0;
		};

		/// <summary>
		/// CallbackTask manages a signal-counting interrupt event.
		///
		/// - Use OnInterrupt() from an ISR to increment the signal count and wake the scheduler.
		/// - The Run() method is called by the scheduler to process the event and notify the listener.
		/// - All accesses to the signal count are guarded for atomicity and ISR safety.
		/// - Multiple interrupts before Run() are accumulated and reported as a count.
		/// - The signal count saturates at MaxValue; further interrupts are ignored until processed.
		/// </summary>
		/// <typeparam name="signal_t">Type used for interrupt signal counting (must be unsigned, e.g., uint8_t, uint16_t).</typeparam>
		template<typename signal_t = uint8_t>
		class CallbackTask final : public DynamicTask
		{
		private:
			/// <summary>
			/// Compile-time check: signal_t must be unsigned.
			/// </summary>
			static_assert((signal_t)-1 > 0, "signal_t must be unsigned");

			/// <summary>
			/// Maximum value for the signal count (saturation limit).
			/// </summary>
			static constexpr signal_t MaxValue = (signal_t)~(signal_t)0;

		private:
			volatile signal_t InterruptSignal = 0;

		private:
			InterruptListener<signal_t>* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			/// <summary>
			/// Attaches an InterruptListener to receive signal count notifications.
			/// </summary>
			/// <param name="listener">Pointer to the listener implementation.</param>
			/// <returns>True if successfully attached, false otherwise.</returns>
			bool AttachListener(InterruptListener<signal_t>* listener)
			{
				// Registers this task with the scheduler using:
				//   delay = 0 (run immediately when triggered)
				//   enabled = false (task starts disabled until an interrupt occurs)
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
			/// <summary>
			/// Called by the scheduler to process the signal interrupt event.
			/// If the signal count is nonzero, notifies the listener and clears the count.
			/// </summary>
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

			/// <summary>
			/// Called from an ISR to increment the signal count and wake the scheduler.
			/// If the count is at MaxValue, further interrupts are ignored until processed.
			/// </summary>
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