#ifndef _HARMONIC_PLATFORM_ATOMIC_h
#define _HARMONIC_PLATFORM_ATOMIC_h

#include "Platform.h"

namespace Harmonic
{
	namespace Platform
	{
		/// <summary>
		/// AtomicGuard provides a scoped, RAII-style critical section for atomic operations.
		///
		/// Usage:
		///   - Create an instance of AtomicGuard on the stack before accessing shared data.
		///   - The constructor disables interrupts or enters a critical section.
		///   - The destructor restores the previous interrupt state or exits the critical section.
		///   - Copy and assignment are deleted to prevent misuse.
		///
		/// Platform-specific behavior:
		///   - AVR: Saves SREG and disables interrupts with cli(); restores SREG on destruction.
		///   - STM32/SAMD: Disables interrupts with noInterrupts(); restores with interrupts().
		///   - FreeRTOS/RTOS: Uses taskENTER_CRITICAL()/taskEXIT_CRITICAL() for thread safety.
		///
		/// Example:
		///   {
		///       Platform::AtomicGuard guard;
		///       // critical section code here
		///   } // interrupts restored here
		/// </summary>
#if defined(ARDUINO_ARCH_AVR)
		class AtomicGuard
		{
			uint8_t sreg_;
		public:
			/// <summary>
			/// Disables interrupts and saves the current status register.
			/// </summary>
			AtomicGuard() { sreg_ = SREG; cli(); }
			/// <summary>
			/// Disables interrupts and saves the current status register.
			/// </summary>
			~AtomicGuard() { SREG = sreg_; }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_SAMD)
		class AtomicGuard 
		{
		public:
			/// <summary>
			/// Disables interrupts.
			/// </summary>
			AtomicGuard() { noInterrupts(); }
			/// <summary>
			/// Enables interrupts.
			/// </summary>
			~AtomicGuard() { interrupts(); }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(HARMONIC_PLATFORM_OS) || defined(FreeRTOS_h)
		class AtomicGuard 
		{
		public:
			/// <summary>
			/// Enters a FreeRTOS critical section (disables task switching and interrupts up to configMAX_SYSCALL_INTERRUPT_PRIORITY).
			/// </summary>
			AtomicGuard() { taskENTER_CRITICAL(); }

			/// <summary>
			/// Exits the FreeRTOS critical section.
			/// </summary> 
			~AtomicGuard() { taskEXIT_CRITICAL(); }
			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#else
#error "No atomic guard defined for this platform"
#endif
	}
}
#endif