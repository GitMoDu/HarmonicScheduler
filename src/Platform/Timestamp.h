#ifndef _HARMONIC_PLATFORM_TIMESTAMP_h
#define _HARMONIC_PLATFORM_TIMESTAMP_h

#include "Platform.h"

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(HARMONIC_PLATFORM_OS)
#include <chrono>
#endif

namespace Harmonic
{
	/// <summary>
	/// Platform specific implementations for timestamp source and idle sleep.
	/// </summary>
	namespace Platform
	{
#if defined(HARMONIC_PLATFORM_OS)
		inline std::chrono::steady_clock::time_point GetStartTimestamp()
		{
			static const std::chrono::steady_clock::time_point startTimestamp = std::chrono::steady_clock::now();
			return startTimestamp;
		}
#endif

		/// <summary>
		/// Get the current time.
		/// </summary>
		/// <returns>Timestamp in milliseconds.</returns>
		inline uint32_t GetTimestamp()
		{
#if defined(ARDUINO)
			return millis();
#elif defined(HARMONIC_PLATFORM_OS)
			return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - GetStartTimestamp()).count());
#else
#error No timestamp source for scheduler.
#endif
		}

		/// <summary>
		/// Gets the current profiler timestamp in microseconds.
		/// </summary>
		/// <returns>Timestamp timestamp in microseconds.</returns>
		inline uint32_t GetProfilerTimestamp()
		{
#if defined(ARDUINO)
			return micros();
#elif defined(HARMONIC_PLATFORM_OS)
			return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - GetStartTimestamp()).count());
#else
#error No timestamp source for profiler.
#endif
		}
	}
}
#endif