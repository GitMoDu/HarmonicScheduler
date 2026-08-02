#ifndef _HARMONIC_PLATFORM_TIMESTAMP_h
#define _HARMONIC_PLATFORM_TIMESTAMP_h

#include "Platform.h"

#if defined(HARMONIC_PLATFORM_OS)
#include <chrono>
#elif defined(HARMONIC_PLATFORM_RTOS)
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <FreeRTOS.h>
#include <task.h>
#endif
#elif defined(ARDUINO)
#include <Arduino.h>
#endif

namespace Harmonic
{
	namespace Platform
	{
#if defined(HARMONIC_PLATFORM_OS)
		/// <summary>
		/// Gets the static start timestamp for the program, used to calculate elapsed time.
		/// </summary>
		/// <returns>The start timestamp as a std::chrono::steady_clock::time_point.</returns>
		inline std::chrono::steady_clock::time_point GetStartTimestamp()
		{
			static const std::chrono::steady_clock::time_point startTimestamp = std::chrono::steady_clock::now();
			return startTimestamp;
		}
#endif

		/// <summary>
		/// Get the current time in milliseconds.
		/// </summary>
		inline uint32_t GetTimestamp()
		{
#if defined(HARMONIC_PLATFORM_RTOS)
			// Converts FreeRTOS ticks to milliseconds using 64-bit integer math to prevent overflow and port-incompatibility.
			return static_cast<uint32_t>((xTaskGetTickCount() * 1000ULL) / configTICK_RATE_HZ);
#elif defined(ARDUINO)
			// Arduino millis() returns milliseconds since the program started.
			return millis();
#elif defined(HARMONIC_PLATFORM_OS)
			// Use std::chrono to get the current time in milliseconds since the program started.
			return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - GetStartTimestamp()).count());
#else
#error "No timestamp source for scheduler."
#endif
		}

		/// <summary>
		/// Gets the current profiler timestamp in microseconds.
		/// </summary>
		inline uint32_t GetProfilerTimestamp()
		{
#if defined(ARDUINO)
			return micros();

#elif defined(HARMONIC_PLATFORM_RTOS)
			// RTOS tick resolution is usually 1ms (1000Hz) or 10ms (100Hz).
			// For microsecond profiling under RTOS, fall back to hardware timer if available.
#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
			return time_us_32();
#elif defined(ESP32)
			return static_cast<uint32_t>(esp_timer_get_time());
#else
			return static_cast<uint32_t>(pdTICKS_TO_MS(xTaskGetTickCount())) * 1000U;
#endif

#elif defined(HARMONIC_PLATFORM_OS)
			return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now() - GetStartTimestamp()).count());

#else
#error "No timestamp source for profiler."
#endif
		}
	}
}

#endif