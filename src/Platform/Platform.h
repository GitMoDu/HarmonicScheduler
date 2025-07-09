#ifndef _HARMONIC_PLATFORM_h
#define _HARMONIC_PLATFORM_h

#include <Arduino.h>

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
#elif defined(WINDOWS)
#define HARMONIC_PLATFORM_OS
#include <thread>
#endif

namespace Harmonic
{
	/// <summary>
	/// TaskId type and count type.
	/// </summary>
	typedef uint_fast8_t task_id_t;

	/// <summary>
	/// Platform specific implementations for timestamp source and idle sleep.
	/// </summary>
	namespace Platform
	{
		/// <summary>
		/// Get the current time.
		/// </summary>
		/// <returns>Timestamp in milliseconds.</returns>
		static uint32_t GetTimestamp()
		{
#if defined(ARDUINO)
			return millis();
#else
#error No timestamp source for scheduler.
#endif
		}

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

#ifdef HARMONIC_PLATFORM_OS
		/// <summary>
		/// Sleep RTOS thread until the next scheduled run.
		/// </summary>
		void IdleSleep(SemaphoreHandle_t& semaphore, const uint32_t sleepDuration)
		{
			// Wait for 1ms until ISR gives the semaphore
			xSemaphoreTake(semaphore, pdMS_TO_TICKS(sleepDuration));
		}
#endif
	}
}
#endif