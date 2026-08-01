#ifndef _HARMONIC_PLATFORM_IDLE_SLEEP_h
#define _HARMONIC_PLATFORM_IDLE_SLEEP_h

#include "Platform.h"

#if defined(HARMONIC_PLATFORM_RTOS) && (defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350))
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#elif defined(HARMONIC_PLATFORM_RTOS) && defined(ARDUINO_ARCH_NRF52)
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <InternalFileSystem.h>
#elif defined(HARMONIC_PLATFORM_RTOS) && (defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266))
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#elif defined(HARMONIC_PLATFORM_OS)
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#endif

namespace Harmonic
{
#if defined(HARMONIC_PLATFORM_OS)
	struct DesktopSemaphore
	{
		std::mutex Mutex;
		std::condition_variable Condition;
		bool Signaled = false;
	};

	using SemaphoreHandle_t = DesktopSemaphore*;
	using BaseType_t = int;

	inline SemaphoreHandle_t xSemaphoreCreateBinary()
	{
		return new DesktopSemaphore();
	}

	inline void vSemaphoreDelete(SemaphoreHandle_t sem)
	{
		delete sem;
	}

	inline bool xSemaphoreTake(SemaphoreHandle_t sem, const uint32_t timeoutMs)
	{
		std::unique_lock<std::mutex> lock(sem->Mutex);
		if (!sem->Signaled)
		{
			if (!sem->Condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return sem->Signaled; }))
			{
				return false;
			}
		}

		sem->Signaled = false;
		return true;
	}

	inline void xSemaphoreGiveFromISR(SemaphoreHandle_t sem, BaseType_t* /*xHigherPriorityTaskWoken*/)
	{
		{
			std::lock_guard<std::mutex> lock(sem->Mutex);
			sem->Signaled = true;
		}

		sem->Condition.notify_one();
	}

	static constexpr BaseType_t pdFALSE = 0;

	inline void portYIELD_FROM_ISR(const BaseType_t) {}
#endif

	/// <summary>
	/// Platform specific implementations for timestamp source and idle sleep.
	/// </summary>
	namespace Platform
	{
		/// <summary>
		/// Sleep device until the next millisecond tick.
		/// </summary>
		static void IdleSleep()
		{
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
			// AVR: sleep until the next interrupt (typically the timer0/millis overflow ISR).
			set_sleep_mode(SLEEP_MODE_IDLE);
			sleep_enable();
			sleep_mode();
			sleep_disable();
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) \
   || defined(ARDUINO_ARCH_STM32)  || defined(ARDUINO_ARCH_SAMD) \
   || defined(CORE_TEENSY) \
   || ((defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)) && !defined(HARMONIC_PLATFORM_RTOS))
			// ARM Cortex-M (STM32, SAMD21/SAMD51, Teensy, bare-metal RP2040/RP2350):
			// WFI suspends the core until the next pending interrupt (typically ARM SysTick).
			asm volatile("wfi");
#else
			// No idle-sleep primitive available on this platform.
			// The scheduler will busy-spin until the next task is due.
			// To enable idle sleep, define a platform-specific IdleSleep() override
			// or supply a semaphore-based overload via HARMONIC_PLATFORM_RTOS.
#endif
		}

#if defined(HARMONIC_PLATFORM_RTOS)
		/// <summary>
		/// Puts the current RTOS thread to sleep until either the specified duration elapses
		/// or an interrupt (ISR) gives the semaphore, whichever comes first.
		/// To avoid waking up late due to RTOS tick granularity, the sleep duration is reduced
		/// by one tick. This ensures the thread wakes up on time or slightly early, never late.
		///
		/// RTOS wake-primitive used per supported core:
		///   - RP2040/RP2350 + FreeRTOS (arduino-pico): xSemaphoreTake / xSemaphoreGiveFromISR
		///       FreeRTOS port ships inside the arduino-pico core; configTICK_RATE_HZ is typically 1000.
		///   - nRF52 (Adafruit/Bluefruit): xSemaphoreTake / xSemaphoreGiveFromISR
		///       FreeRTOS port is bundled with the Adafruit nRF52 Arduino core.
		///   - ESP32 (arduino-esp32): xSemaphoreTake / xSemaphoreGiveFromISR
		///       FreeRTOS is the native OS on ESP32; all Arduino tasks run inside FreeRTOS tasks.
		///   - ESP8266 (arduino-esp8266 with RTOS SDK): xSemaphoreTake / xSemaphoreGiveFromISR
		///       Only applies when HARMONIC_PLATFORM_RTOS is explicitly defined on ESP8266.
		/// </summary>
		/// <param name="semaphore">
		/// Reference to a binary semaphore used for waking the thread from an ISR.
		/// </param>
		/// <param name="sleepDuration">
		/// Desired sleep duration in milliseconds.
		/// </param>
		void IdleSleep(SemaphoreHandle_t semaphore, const uint32_t sleepDuration)
		{
			static constexpr uint32_t tickPeriod = (1000 / configTICK_RATE_HZ);

			if (sleepDuration >= tickPeriod)
			{
				// Block the thread until either:
				// 1. The semaphore is given from an ISR (interrupt), or
				// 2. The (sleepDuration - 1 tick) timeout elapses.
				// Subtracting one tick prevents oversleeping due to RTOS tick rounding.
				xSemaphoreTake(semaphore, pdMS_TO_TICKS(sleepDuration - tickPeriod));
			}
		}
#elif defined(HARMONIC_PLATFORM_OS)
		/// <summary>
		/// Puts the current thread to sleep until either the specified duration elapses
		/// or another thread signals the semaphore, whichever comes first.
		/// </summary>
		/// <param name="semaphore">
		/// Reference to a binary semaphore used for waking the thread.
		/// </param>
		/// <param name="sleepDuration">
		/// Desired sleep duration in milliseconds.
		/// </param>
		void IdleSleep(SemaphoreHandle_t semaphore, const uint32_t sleepDuration)
		{
			if (sleepDuration != 0)
			{
				xSemaphoreTake(semaphore, sleepDuration);
			}
		}
#endif
	}
}
#endif