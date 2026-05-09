#ifndef _HARMONIC_PLATFORM_ATOMIC_h
#define _HARMONIC_PLATFORM_ATOMIC_h

#include "Platform.h"

#if defined(HARMONIC_PLATFORM_OS)
#include <mutex>
#elif defined(HARMONIC_PLATFORM_RTOS)
#include <FreeRTOS.h>
#include <task.h>
#elif (defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)) && !defined(HARMONIC_PLATFORM_RTOS)
#include <hardware/sync.h>
#endif

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
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
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
#elif defined(ARDUINO_ARCH_STM32) || defined(CORE_TEENSY)
		class AtomicGuard
		{
			uint32_t primask_;
		public:
			/// <summary>
			/// Disables interrupts and saves the previous global interrupt state.
			/// </summary>
			AtomicGuard()
			{
				asm volatile("mrs %0, primask" : "=r"(primask_));
				asm volatile("cpsid i");
			}

			/// <summary>
			/// Restores the previous global interrupt state.
			/// </summary>
			~AtomicGuard()
			{
				if (primask_ == 0)
					asm volatile("cpsie i");
			}

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(HARMONIC_PLATFORM_OS)
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
			/// Enters a process-local critical section for desktop platforms.
			/// </summary>
			AtomicGuard()
				: lock_(Mutex())
			{}

			~AtomicGuard() = default;

			AtomicGuard(const AtomicGuard&) = delete;
			AtomicGuard& operator=(const AtomicGuard&) = delete;
		};
#elif defined(HARMONIC_PLATFORM_RTOS)
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
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
		class AtomicGuard
		{
			uint32_t interrupts_;

		public:
			/// <summary>
			/// Disables interrupts and saves the previous interrupt state.
			/// </summary>
			AtomicGuard()
				: interrupts_(save_and_disable_interrupts())
			{}

			/// <summary>
			/// Restores the previous interrupt state.
			/// </summary>
			~AtomicGuard()
			{
				restore_interrupts(interrupts_);
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