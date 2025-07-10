#ifndef _HARMONIC_INTERRUPT_CALLBACK_TASK_h
#define _HARMONIC_INTERRUPT_CALLBACK_TASK_h

#include "DynamicTask.h"

namespace Harmonic
{
	namespace InterruptCallback
	{
		struct NoTimestampSource
		{
			static uint32_t Get()
			{
				return 0;
			}
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

		struct InterruptListener
		{
			virtual void OnInterrupt(const uint32_t interruptTimestamp) {}
			virtual void OnErrorInterrupt(const uint32_t interruptTimestamp, const uint8_t interruptions) {}
		};
	}

	/// <summary>
	/// Harmonic cooperative base for delivering interrupt events to the scheduler loop.
	/// 
	/// This task bridges hardware/software interrupts to the cooperative scheduler by capturing
	/// interrupt events (in interrupt context) and delivering them as virtual callbacks
	/// (`OnInterrupt` or `OnErrorInterrupt`) to a registered listener during the scheduler loop,
	/// along with a timestamp from the specified `TimestampSource`.
	///
	/// If multiple interrupts occur before the previous one is handled, the listener's
	/// `OnErrorInterrupt` is called with the count. Overflow is not checked; if exactly 255
	/// interrupts occur before delivery, this edge case is not handled.
	/// 
	/// Usage: Inherit or instantiate, call `OnInterrupt()` from the ISR, and implement a listener.
	/// </summary>
	/// <typeparam name="TimestampSource">Provides the timestamp for each interrupt event.</typeparam>
	template<typename TimestampSource = InterruptCallback::MicrosTimestampSource>
	class InterruptCallbackTask final : public DynamicTask
	{
	private:
		volatile uint32_t InterruptTimestamp = 0;
		volatile uint8_t InterruptFlags = 0;

	private:
		InterruptCallback::InterruptListener* Listener = nullptr;

	public:
		InterruptCallbackTask(TaskRegistry& registry) : DynamicTask(registry) {}

		bool AttachListener(InterruptCallback::InterruptListener* listener)
		{
			if (AttachTask(0, false))
			{
				Listener = listener;

				return true;
			}

			return false;
		}

	public:
		void Run() final
		{
			noInterrupts();
			const uint32_t timestamp = InterruptTimestamp;
			const uint8_t interruptFlags = InterruptFlags;
			InterruptFlags = 0;
			interrupts();

			if (interruptFlags > 0
				&& Listener != nullptr)
			{
				if (interruptFlags > 1)
				{
					Listener->OnErrorInterrupt(timestamp, interruptFlags);
				}
				else
				{
					Listener->OnInterrupt(timestamp);
				}
			}

			noInterrupts();
			const bool interruptPending = InterruptFlags > 0;
			interrupts();

			SetEnabled(interruptPending);
		}

		void OnInterrupt()
		{
			if (InterruptFlags == 0)
			{
				InterruptTimestamp = TimestampSource::Get();
				InterruptFlags++;
				WakeFromISR();
			}
			else
			{
				InterruptFlags++;
			}
		}
	};
}
#endif