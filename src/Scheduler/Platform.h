#ifndef _HARMONIC_PLATFORM_h
#define _HARMONIC_PLATFORM_h

#if defined(ARDUINO)
#include <Arduino.h>

#if defined(ARDUINO_ARCH_RP2040)
#include <_freertos.h>
#endif
#if defined(ARDUINO_ARCH_AVR)
#include <avr/power.h>
#include <avr/sleep.h>
#endif

#endif

/// <summary>
/// Platform specific implementations for timestamp source and idle sleep.
/// </summary>
namespace Harmonic::Platform
{
#if defined(ARDUINO)
	static const uint32_t GetTimestamp()
	{
		return millis();
	}

	/// <summary>
	/// Sleep device until the next ms tick.
	/// </summary>
	static void IdleSleep()
	{
#if defined(ARDUINO_ARCH_AVR)
		// No RTOS, sleep until next interrupt. (most likely timer0/millis).
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		sleep_mode();
		sleep_disable();
#elif defined(ARDUINO_ARCH_STM32F1) || defined(CORE_TEENSY)
		// No RTOS, sleep until next interrupt. (most likely ARM systick).
		asm("wfi");
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
		// Yield to RTOS and wait until thread is running again.
		yield();
#elif defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_RP2040)
		// Yield to RTOS for 1ms.
		vTaskDelay(pdMS_TO_TICKS(1));
#else
		// No sleep idle implementation.
#endif
	}

#else
#error No timestamp source for scheduler.
#endif
}
#endif