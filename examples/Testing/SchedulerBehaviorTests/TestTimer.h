#ifndef _TESTTIMER_h
#define _TESTTIMER_h

#include <stdint.h>
#include "Platform/Platform.h"

#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
#include <pico/time.h>
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#include <HardwareTimer.h>
#endif

namespace Harmonic
{
	namespace TestTasks
	{

		// Fixed, simple timer interface used by tests.
		class TestTimer
		{
		public:
			using Callback = void(*)(void);

			TestTimer()
				: cb_(nullptr)
			{}

			// Platform-specific timing and identifiers are provided here so callers
			// do not need any #if branches.
#if defined(ARDUINO_ARCH_AVR)
			static constexpr uint16_t Timer1Prescaler = 64;
			static constexpr uint16_t Timer1CompareValue = (F_CPU / Timer1Prescaler) / 10;
			static constexpr uint32_t ExpectedDurationMicros = (uint64_t(Timer1CompareValue) * Timer1Prescaler * 1000000UL) / F_CPU;
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#if defined(F_CPU)
			static constexpr uint32_t TimerPrescaler = (F_CPU / 10000) - 1; // ~10kHz
#endif
			static constexpr uint16_t TimerOverflow = 10000;     // 1s (10kHz * 1s)
			static constexpr uint32_t ExpectedDurationMicros = 1000000; // 1s in microseconds
			static constexpr uint8_t TestTimerIndex = 2;
			static constexpr uint8_t TestTimerChannel = 0;
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
			static constexpr uint32_t ExpectedDurationMicros = 1000000; // 1s in microseconds
#else
			static constexpr uint32_t ExpectedDurationMicros = 0;
#endif

			void SetCallback(Callback cb)
			{
				cb_ = cb;
			}

			// Setup the timer to fire after 'ms' milliseconds. Returns true if timer started.
			bool SetupMs(uint32_t ms)
			{
#if defined(ARDUINO_ARCH_AVR)
				// Use Timer1 with prescaler 64. Compute OCR1A for requested ms.
				const uint32_t prescaler = 64;
				uint64_t ocr = (uint64_t)ms * (uint64_t)F_CPU / (prescaler * 1000ULL);
				if (ocr == 0 || ocr > 0xFFFFu)
					return false;

				Platform::AtomicGuard guard;
				// Disable and clear
				TIMSK1 &= ~(1 << OCIE1A);
				TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
				TIFR1 |= (1 << OCF1A);

				TCCR1A = 0;
				TCCR1B = 0;
				TCNT1 = 0;
				OCR1A = (uint16_t)ocr;
				TCCR1B |= (1 << WGM12); // CTC
				TIMSK1 |= (1 << OCIE1A);
				// Start with prescaler 64
				TCCR1B |= (1 << CS11) | (1 << CS10);
				return true;
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
				// Use HardwareTimer index 2, channel 0, base 10kHz and overflow = ms*10
				Disable();
				timer_ = HardwareTimer(2);
				timer_.init();
				const uint32_t baseHz = 10000; // 10kHz base
				uint32_t prescaler = (timer_.getClockSpeed() / baseHz) - 1;
				timer_.setPrescaleFactor(prescaler);
				uint32_t overflow = ms * 10u;
				if (overflow == 0) overflow = 1;
				if (overflow > 0xFFFFu) overflow = 0xFFFFu;
				timer_.setOverflow((uint16_t)overflow);
				timer_.refresh();
				timer_.attachInterrupt(0, cb_);
				timer_.resume();
				return true;
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
				// Use pico repeating timer as a one-shot by returning false from callback.
				Disable();
				int32_t intervalMs = int32_t(ms);
				if (intervalMs <= 0)
					return false;
				return add_repeating_timer_ms(intervalMs, &TestTimer::PicoCallback, this, &picoTimer_);
#else
				(void)ms; (void)cb_;
				// No timer available on this platform.
				return false;
#endif
			}

			void Disable()
			{
#if defined(ARDUINO_ARCH_AVR)
				Platform::AtomicGuard guard;
				TIMSK1 &= ~(1 << OCIE1A);
				TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
				TIFR1 |= (1 << OCF1A);
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
				timer_.pause();
				timer_.detachInterrupt(0);
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
				cancel_repeating_timer(&picoTimer_);
#else
				// no-op
#endif
			}

		private:
			Callback cb_;
#if defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
			HardwareTimer timer_{ 2 };
#endif
#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
			repeating_timer_t picoTimer_{};
			static bool PicoCallback(repeating_timer_t* rt)
			{
				TestTimer* self = reinterpret_cast<TestTimer*>(rt->user_data);
				if (self && self->cb_)
					self->cb_();
				return false; // one-shot
			}
#endif
		};

	}
}

#endif
