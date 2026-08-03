#ifndef _HARMONIC_PLATFORM_ATOMIC_h
#define _HARMONIC_PLATFORM_ATOMIC_h

#include "Rtos.h"

// Header includes by platform hierarchy. 
#if defined(HARMONIC_PLATFORM_OS)
#include <mutex>
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350) || defined(ARDUINO_RASPBERRY_PI_PICO2)
#include <hardware/sync.h>
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
#include <avr/interrupt.h>
#endif

namespace Harmonic
{
	namespace Platform
	{
		/// <summary>
		/// Provides a scoped RAII critical section for thread-safe and ISR-safe atomic operations.
		///
		/// Usage:
		///   {
		///       Platform::AtomicGuard guard;
		///       // Critical section code
		///   } // Interrupt/scheduler state restored automatically on destruction
		///
		/// Platform-specific behavior:
		///   - AVR (8-bit): Saves SREG, disables interrupts (cli()), and restores SREG on exit.
		///   - ARM Cortex-M: Saves PRIMASK, executes __disable_irq(), and restores PRIMASK on exit.
		///   - RP2040 / RP2350: Uses SDK save_and_disable_interrupts() / restore_interrupts().
		///   - FreeRTOS / RTOS: Uses taskENTER_CRITICAL() / taskEXIT_CRITICAL() (spinlock-backed on SMP/dual-core).
		/// </summary>
#if defined(HARMONIC_PLATFORM_OS)
		class AtomicGuard
		{
			static std::recursive_mutex& Mutex()
			{
				static std::recursive_mutex mutex;
				return mutex;
			}

			std::lock_guard<std::recursive_mutex> lock_;

		public:
			/// <summary>
			/// Enters a critical section, acquiring the recursive mutex.
			/// </summary>
			AtomicGuard() : lock_(Mutex())
			{}

			/// <summary>
			/// Exits the critical section, releasing the recursive mutex.
			/// </summary>
			~AtomicGuard() = default;

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(HARMONIC_PLATFORM_RTOS) || defined(FreeRTOS_H)
		class AtomicGuard
		{
			UBaseType_t uxSavedInterruptStatus_;

		public:
			/// <summary>
			/// Enters a critical section, saving the current interrupt state.
			/// </summary>
			AtomicGuard()
			{
				uxSavedInterruptStatus_ = taskENTER_CRITICAL_FROM_ISR();
			}

			/// <summary>
			/// Restores the previous interrupt state, exiting the critical section.
			/// </summary>
			~AtomicGuard()
			{
				taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus_);
			}

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350) || defined(ARDUINO_RASPBERRY_PI_PICO2)
		class AtomicGuard
		{
			uint32_t interrupts_;

		public:
			/// <summary>
			/// Enters a critical section, saving the current interrupt state.
			/// </summary>
			AtomicGuard()
				: interrupts_(save_and_disable_interrupts())
			{}

			/// <summary>
			/// Restores the previous interrupt state, exiting the critical section.
			/// </summary>
			~AtomicGuard()
			{
				restore_interrupts(interrupts_);
			}

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
		class AtomicGuard
		{
			uint8_t sreg_;

		public:
			/// <summary>
			/// Disables interrupts and saves the current status register.
			/// </summary>
			AtomicGuard() { sreg_ = SREG; cli(); }

			/// <summary>
			/// Restores the previous interrupt state.
			/// </summary>
			~AtomicGuard() { SREG = sreg_; }

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
		class AtomicGuard
		{
			bool wasEnabled_;

		public:
			/// <summary>
			/// Disables interrupts and saves the previous global interrupt state.
			/// </summary>
			AtomicGuard()
			{
				asm volatile("mrs %0, primask" : "=r"(wasEnabled_));
				nvic_globalirq_disable();
			}

			/// <summary>
			/// Restores the previous global interrupt state.
			/// </summary>
			~AtomicGuard()
			{
				if (wasEnabled_ == 0)
					nvic_globalirq_enable();
			}

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY) || defined(ARDUINO_ARCH_NRF52)
		class AtomicGuard
		{
			uint32_t primask_;

		public:
			/// <summary>
			/// Enters a critical section, saving the current interrupt state.
			/// </summary>
			AtomicGuard()
			{
				asm volatile("mrs %0, primask" : "=r"(primask_));
				asm volatile("cpsid i" ::: "memory"); // Memory fence prevents reordering
			}

			/// <summary>
			/// Restores the previous interrupt state, exiting the critical section.
			/// </summary>
			~AtomicGuard()
			{
				if (primask_ == 0)
					asm volatile("cpsie i" ::: "memory");
			}

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#else
#error "No atomic guard defined for this platform"
#endif
	}
}

#endif