#ifndef _HARMONIC_PLATFORM_IDLE_SLEEP_h
#define _HARMONIC_PLATFORM_IDLE_SLEEP_h

#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
#define HARMONIC_PLATFORM_OS
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#elif defined(ARDUINO_ARCH_NRF52)
#define HARMONIC_PLATFORM_OS
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <InternalFileSystem.h>
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#define HARMONIC_PLATFORM_OS
#elif defined(ARDUINO_ARCH_AVR)
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#elif defined(WINDOWS)
#define HARMONIC_PLATFORM_OS
#include <thread>
#endif

namespace Harmonic
{
#if defined(WINDOWS)
	using SemaphoreHandle_t = struct
	{
		std::mutex mtx;
		std::condition_variable cv;
		bool signaled = false;
	};

	SemaphoreHandle_t* xSemaphoreCreateBinary()
	{
		return new SemaphoreHandle_t();
	}

	void vSemaphoreDelete(SemaphoreHandle_t* sem)
	{
		delete sem;
	}

	bool xSemaphoreTake(SemaphoreHandle_t* sem, uint32_t timeout_ms)
	{
		std::unique_lock<std::mutex> lock(sem->mtx);
		if (!sem->signaled) {
			if (timeout_ms == 0) {
				sem->cv.wait(lock, [&] { return sem->signaled; });
			}
			else {
				if (!sem->cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return sem->signaled; }))
					return false;
			}
		}
		sem->signaled = false;
		return true;
	}

	void xSemaphoreGiveFromISR(SemaphoreHandle_t* sem, int* /*xHigherPriorityTaskWoken*/)
	{
		{
			std::lock_guard<std::mutex> lock(sem->mtx);
			sem->signaled = true;
		}
		sem->cv.notify_one();
	}

	uint32_t pdMS_TO_TICKS(uint32_t ms) { return ms; }
	static constexpr int pdFALSE = 0;
	void portYIELD_FROM_ISR(int) {}
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
#if defined(ARDUINO_ARCH_AVR)
			// No RTOS, sleep until next interrupt. (most likely timer0/millis).
			set_sleep_mode(SLEEP_MODE_IDLE);
			sleep_enable();
			sleep_mode();
			sleep_disable();
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(CORE_TEENSY)
			// No RTOS, sleep until next interrupt. (most likely ARM systick).
			asm("wfi");
#endif
		}

#if defined(HARMONIC_PLATFORM_OS)
		/// <summary>
		/// Puts the current RTOS thread to sleep until either the specified duration elapses
		/// or an interrupt (ISR) gives the semaphore, whichever comes first.
		/// To avoid waking up late due to RTOS tick granularity, the sleep duration is reduced
		/// by one tick. This ensures the thread wakes up on time or slightly early, never late.
		/// </summary>
		/// <param name="semaphore">
		/// Reference to a binary semaphore used for waking the thread from an ISR.
		/// </param>
		/// <param name="sleepDuration">
		/// Desired sleep duration in milliseconds.
		/// </param>
		void IdleSleep(SemaphoreHandle_t& semaphore, const uint32_t sleepDuration)
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
#endif
	}
}
#endif