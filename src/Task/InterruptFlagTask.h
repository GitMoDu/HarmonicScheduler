#ifndef _HARMONIC_INTERRUPT_FLAG_TASK_h
#define _HARMONIC_INTERRUPT_FLAG_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptFlag
	{
		/// <summary>
		/// Interface for receiving flag-based interrupt events from CallbackTask.
		/// </summary>
		struct InterruptListener
		{
			/// <summary>
			/// Called from main context (loop) when an interrupt was triggered.
			/// </summary>
			virtual void OnFlagInterrupt() = 0;
		};

		/// <summary>
		/// CallbackTask manages a single flag-based interrupt event.
		/// 
		/// - Use OnInterrupt() from an ISR to set the interrupt flag and wake the scheduler.
		/// - The Run() method is called by the scheduler to process the event and notify the listener.
		/// - All accesses to the interrupt flag are guarded for atomicity and ISR safety.
		/// - Only one interrupt event is tracked at a time; repeated interrupts before Run() are coalesced.
		/// </summary>
		class CallbackTask final : public DynamicTask
		{
		private:
			volatile bool InterruptFlag = false;

		private:
			InterruptListener* Listener = nullptr;

		public:
			CallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

			/// <summary>
			/// Attaches an InterruptListener to receive interrupt notifications.
			/// </summary>
			/// <param name="listener">Pointer to the listener implementation.</param>
			/// <returns>True if successfully attached, false otherwise.</returns>
			bool AttachListener(InterruptListener* listener)
			{
				// Registers this task with the scheduler using:
				//   delay = 0 (run immediately when triggered)
				//   enabled = false (task starts disabled until an interrupt occurs)
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

				const bool interruptPending = InterruptFlag;
				SetEnabled(interruptPending);
			}

			/// <summary>
			/// Called from an ISR to set the interrupt flag and wake the scheduler.
			/// If the flag is already set, does nothing (coalesces repeated interrupts).
			/// </summary>
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