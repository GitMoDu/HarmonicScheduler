#ifndef _HARMONIC_PLATFORM_ATOMIC_h
#define _HARMONIC_PLATFORM_ATOMIC_h

#include "Platform.h"

namespace Harmonic
{
	namespace Platform
	{
#if defined(ARDUINO_ARCH_AVR)
		class AtomicGuard {
			uint8_t sreg_;
		public:
			AtomicGuard() { sreg_ = SREG; cli(); }
			~AtomicGuard() { SREG = sreg_; }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_SAMD)
		class AtomicGuard {
		public:
			AtomicGuard() { noInterrupts(); }
			~AtomicGuard() { interrupts(); }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(HARMONIC_PLATFORM_OS) || defined(FreeRTOS_h)
		// Use RTOS primitives or leave as a no-op if RTOS handles atomicity
		class AtomicGuard {
		public:
			AtomicGuard() { taskENTER_CRITICAL() }
			~AtomicGuard() { taskEXIT_CRITICAL() }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#else
#error "No atomic guard defined for this platform"
#endif
	}
}
#endif